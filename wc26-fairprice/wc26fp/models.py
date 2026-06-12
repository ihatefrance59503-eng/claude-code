"""Models: calibrated multinomial logistic regression (the interpretable
spine), gradient boosting on identical features, and a log-loss-weighted
ensemble. Optional market blend (shrink toward de-vigged odds by lambda).
"""
from __future__ import annotations

import pickle
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.calibration import CalibratedClassifierCV
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import log_loss
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler
from scipy.optimize import minimize

from .config import SEED, TRAIN_END, VAL_END, DATA_DIR
from .features import FEATURES, LABEL, build_features, feature_matrix

MODEL_PATH = DATA_DIR / "models.pkl"
CLASSES = np.array([0.0, 1.0, 2.0])  # home / draw / away


def time_splits(df: pd.DataFrame) -> tuple[pd.DataFrame, pd.DataFrame, pd.DataFrame]:
    """Strict time-based split. No shuffling, ever."""
    played = df.dropna(subset=[LABEL])
    train = played[played["date"] <= TRAIN_END]
    val = played[(played["date"] > TRAIN_END) & (played["date"] <= VAL_END)]
    test = played[played["date"] > VAL_END]
    return train, val, test


def _frozen(est):
    """Wrap a fitted estimator for post-hoc calibration (sklearn >= 1.6)."""
    try:
        from sklearn.frozen import FrozenEstimator
        return FrozenEstimator(est)
    except ImportError:  # older sklearn: cv='prefit'
        return est


def _calibrate(est, X_val: pd.DataFrame, y_val: np.ndarray):
    """Pick isotonic vs Platt (sigmoid) by validation log loss."""
    best, best_ll, best_name = est, log_loss(y_val, est.predict_proba(X_val),
                                             labels=CLASSES), "none"
    for method in ("isotonic", "sigmoid"):
        try:
            cal = CalibratedClassifierCV(_frozen(est), method=method)
            cal.fit(X_val, y_val)
            ll = log_loss(y_val, cal.predict_proba(X_val), labels=CLASSES)
            if ll < best_ll:
                best, best_ll, best_name = cal, ll, method
        except Exception:  # noqa: BLE001 — calibration is best-effort
            continue
    return best, best_name


@dataclass
class Ensemble:
    """Holds the fitted component models and blend weights."""
    lr: object = None
    gbm: object = None
    weights: np.ndarray = field(default_factory=lambda: np.array([0.5, 0.5]))
    calibration: dict = field(default_factory=dict)
    lr_coefficients: pd.DataFrame | None = None
    market_lambda: float = 0.0

    def predict_proba(self, X: pd.DataFrame,
                      market_probs: np.ndarray | None = None) -> np.ndarray:
        parts = [m.predict_proba(X) for m in (self.lr, self.gbm)]
        p = sum(w * part for w, part in zip(self.weights, parts))
        p = p / p.sum(axis=1, keepdims=True)
        if market_probs is not None and self.market_lambda > 0:
            p = (1 - self.market_lambda) * p + self.market_lambda * market_probs
            p = p / p.sum(axis=1, keepdims=True)
        return p


def train_models(df_feats: pd.DataFrame, verbose: bool = True) -> Ensemble:
    train, val, _ = time_splits(df_feats)
    X_tr, y_tr = feature_matrix(train), train[LABEL].to_numpy()
    X_va, y_va = feature_matrix(val), val[LABEL].to_numpy()

    # 1 — multinomial logistic regression (interpretable spine)
    lr_pipe = Pipeline([
        ("scaler", StandardScaler()),
        ("lr", LogisticRegression(max_iter=3000, C=1.0, random_state=SEED)),
    ])
    lr_pipe.fit(X_tr, y_tr)
    lr_cal, lr_method = _calibrate(lr_pipe, X_va, y_va)

    coefs = pd.DataFrame(
        lr_pipe.named_steps["lr"].coef_.T,
        index=FEATURES, columns=["home_win", "draw", "away_win"],
    )

    # 2 — gradient boosting on identical features
    import lightgbm as lgb
    gbm = lgb.LGBMClassifier(
        objective="multiclass", num_class=3, n_estimators=400,
        learning_rate=0.03, num_leaves=31, min_child_samples=50,
        subsample=0.8, colsample_bytree=0.8, random_state=SEED, verbose=-1,
    )
    gbm.fit(X_tr, y_tr)
    gbm_cal, gbm_method = _calibrate(gbm, X_va, y_va)

    # 3 — blend weights: minimise validation log loss on the simplex
    p_lr = lr_cal.predict_proba(X_va)
    p_gbm = gbm_cal.predict_proba(X_va)

    def blend_ll(w0: np.ndarray) -> float:
        w = np.clip(w0, 0, 1)
        w = w / w.sum() if w.sum() > 0 else np.array([0.5, 0.5])
        p = w[0] * p_lr + w[1] * p_gbm
        return log_loss(y_va, p / p.sum(axis=1, keepdims=True), labels=CLASSES)

    res = minimize(blend_ll, np.array([0.5, 0.5]), method="Nelder-Mead")
    w = np.clip(res.x, 0, 1)
    w = w / w.sum()

    ens = Ensemble(lr=lr_cal, gbm=gbm_cal, weights=w,
                   calibration={"lr": lr_method, "gbm": gbm_method},
                   lr_coefficients=coefs)
    if verbose:
        print(f"  LR calibration: {lr_method} | GBM calibration: {gbm_method}")
        print(f"  Blend weights:  LR={w[0]:.3f}  GBM={w[1]:.3f}")
        print(f"  Val log loss:   LR={log_loss(y_va, p_lr, labels=CLASSES):.4f}  "
              f"GBM={log_loss(y_va, p_gbm, labels=CLASSES):.4f}  "
              f"ensemble={blend_ll(w):.4f}")
    return ens


def tune_market_lambda(ens: Ensemble, df_feats: pd.DataFrame,
                       odds: pd.DataFrame) -> tuple[float, str]:
    """Find lambda in [0,1] minimising validation log loss when shrinking
    toward the de-vigged market. Honest output: if the market dominates
    (lambda -> 1), say so."""
    from .markets import devig_shin

    _, val, _ = time_splits(df_feats)
    merged = val.merge(odds, on=["date", "home_team", "away_team"], how="inner")
    if len(merged) < 50:
        return 0.0, "market blend untunable: <50 validation matches with odds"
    X = feature_matrix(merged)
    y = merged[LABEL].to_numpy()
    mp = np.vstack([
        devig_shin([r.odds_home, r.odds_draw, r.odds_away])
        for r in merged.itertuples()
    ])
    p_model = ens.predict_proba(X)
    lambdas = np.linspace(0, 1, 21)
    lls = [log_loss(y, (1 - l) * p_model + l * mp, labels=CLASSES) for l in lambdas]
    best = float(lambdas[int(np.argmin(lls))])
    note = (f"lambda*={best:.2f}: market blend helps"
            if best > 0 else "lambda*=0: market blend does not help")
    if best >= 0.9:
        note += " — the market dominates the model; treat model edges sceptically"
    return best, note


def save_ensemble(ens: Ensemble) -> None:
    MODEL_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(MODEL_PATH, "wb") as fh:
        pickle.dump(ens, fh)


def load_ensemble() -> Ensemble:
    with open(MODEL_PATH, "rb") as fh:
        return pickle.load(fh)
