"""Evaluation with integrity gates.

Reports log loss, Brier, RPS on the held-out test set for every model,
an Elo-only baseline, a base-rates dummy, and the de-vigged market.
PASS requires beating the market on log loss with paired-bootstrap p<0.05.
No market data => automatic FAIL and the value engine stays in no-bet mode.
"""
from __future__ import annotations

from dataclasses import dataclass

import numpy as np
import pandas as pd
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import log_loss
from sklearn.pipeline import Pipeline
from sklearn.preprocessing import StandardScaler

from .config import SEED, DATA_DIR
from .features import LABEL, feature_matrix
from .markets import devig_proportional, devig_shin
from .models import CLASSES, Ensemble, time_splits

VERDICT_PATH = DATA_DIR / "verdict.txt"


# --------------------------------------------------------------------------- #
# Metrics
# --------------------------------------------------------------------------- #

def brier_multiclass(y: np.ndarray, p: np.ndarray) -> float:
    onehot = np.eye(3)[y.astype(int)]
    return float(np.mean(np.sum((p - onehot) ** 2, axis=1)))


def rps(y: np.ndarray, p: np.ndarray) -> float:
    """Ranked probability score for the ordered outcome home>draw>away."""
    onehot = np.eye(3)[y.astype(int)]
    cum_p = np.cumsum(p, axis=1)
    cum_o = np.cumsum(onehot, axis=1)
    return float(np.mean(np.sum((cum_p - cum_o) ** 2, axis=1) / 2.0))


def _per_match_log_loss(y: np.ndarray, p: np.ndarray) -> np.ndarray:
    eps = 1e-15
    return -np.log(np.clip(p[np.arange(len(y)), y.astype(int)], eps, None))


def paired_bootstrap_p(y: np.ndarray, p_a: np.ndarray, p_b: np.ndarray,
                       n_boot: int = 2000, seed: int = SEED) -> float:
    """P(model A is NOT better than model B on log loss), paired bootstrap.
    Small p => A reliably better than B."""
    la = _per_match_log_loss(y, p_a)
    lb = _per_match_log_loss(y, p_b)
    diff = la - lb                      # negative = A better
    rng = np.random.default_rng(seed)
    idx = rng.integers(0, len(diff), size=(n_boot, len(diff)))
    boot_means = diff[idx].mean(axis=1)
    return float(np.mean(boot_means >= 0))


# --------------------------------------------------------------------------- #
# Baselines
# --------------------------------------------------------------------------- #

def dummy_probs(train: pd.DataFrame, n: int) -> np.ndarray:
    rates = train[LABEL].value_counts(normalize=True).reindex([0.0, 1.0, 2.0]).fillna(0)
    return np.tile(rates.to_numpy(), (n, 1))


def elo_only_probs(train: pd.DataFrame, test: pd.DataFrame) -> np.ndarray:
    cols = ["elo_diff", "neutral"]
    pipe = Pipeline([
        ("scaler", StandardScaler()),
        ("lr", LogisticRegression(max_iter=2000, random_state=SEED)),
    ])
    pipe.fit(train[cols].fillna(0), train[LABEL])
    return pipe.predict_proba(test[cols].fillna(0))


def market_probs(test: pd.DataFrame, odds: pd.DataFrame,
                 method: str = "shin") -> tuple[np.ndarray | None, pd.Index]:
    """De-vigged market probabilities for the test rows that have odds."""
    merged = test.reset_index().merge(
        odds, on=["date", "home_team", "away_team"], how="inner")
    if merged.empty:
        return None, pd.Index([])
    devig = devig_shin if method == "shin" else devig_proportional
    p = np.vstack([devig([r.odds_home, r.odds_draw, r.odds_away])
                   for r in merged.itertuples()])
    return p, pd.Index(merged["index"])


# --------------------------------------------------------------------------- #
# Verdict
# --------------------------------------------------------------------------- #

@dataclass
class Verdict:
    passed: bool
    reason: str
    table: pd.DataFrame

    @property
    def banner(self) -> str:
        if self.passed:
            return ("VERDICT: PASS — ensemble beats the de-vigged market "
                    f"(paired bootstrap p<0.05). {self.reason}")
        return ("VERDICT: FAIL — value signals unreliable; use as fair-price "
                f"reference only. {self.reason}\n"
                "Value engine locked to NO-BET mode "
                "(override with --override-verdict).")


def evaluate(ens: Ensemble, df_feats: pd.DataFrame,
             odds: pd.DataFrame | None = None) -> Verdict:
    train, val, test = time_splits(df_feats)
    if test.empty:
        raise RuntimeError("empty test split — refresh the data")
    y = test[LABEL].to_numpy()
    X = feature_matrix(test)

    rows: dict[str, np.ndarray] = {
        "Dummy (base rates)": dummy_probs(train, len(test)),
        "Elo-only baseline": elo_only_probs(pd.concat([train, val]), test),
        "Logistic regression": ens.lr.predict_proba(X),
        "Gradient boosting": ens.gbm.predict_proba(X),
        "Ensemble": ens.predict_proba(X),
    }

    # Market benchmark (only on the matched subset, compared like-for-like)
    market_note = ""
    p_market_sub = None
    if odds is not None and len(odds):
        for method in ("proportional", "shin"):
            p_mkt, idx = market_probs(test, odds, method)
            if p_mkt is not None:
                sub = test.loc[idx]
                rows[f"Market de-vig ({method})"] = ("SUBSET", p_mkt, idx)
                if method == "shin":
                    p_market_sub = (p_mkt, idx)
        if p_market_sub is None:
            market_note = "odds file present but no rows matched test fixtures"
    else:
        market_note = ("no historical odds source reachable/provided "
                       "(see data/cache/intl_odds.csv in README)")

    records = []
    for name, val_ in rows.items():
        if isinstance(val_, tuple) and val_[0] == "SUBSET":
            _, p, idx = val_
            ysub = test.loc[idx, LABEL].to_numpy()
            records.append({
                "model": name, "n": len(ysub),
                "log_loss": log_loss(ysub, p, labels=CLASSES),
                "brier": brier_multiclass(ysub, p), "rps": rps(ysub, p),
            })
        else:
            records.append({
                "model": name, "n": len(y),
                "log_loss": log_loss(y, val_, labels=CLASSES),
                "brier": brier_multiclass(y, val_), "rps": rps(y, val_),
            })
    table = pd.DataFrame(records).set_index("model").round(4)

    # ---- gate ----
    if p_market_sub is None:
        verdict = Verdict(False, f"market benchmark UNAVAILABLE: {market_note}", table)
    else:
        p_mkt, idx = p_market_sub
        ysub = test.loc[idx, LABEL].to_numpy()
        Xsub = feature_matrix(test.loc[idx])
        p_ens = ens.predict_proba(Xsub)
        ll_ens = log_loss(ysub, p_ens, labels=CLASSES)
        ll_mkt = log_loss(ysub, p_mkt, labels=CLASSES)
        p_val = paired_bootstrap_p(ysub, p_ens, p_mkt)
        if ll_ens < ll_mkt and p_val < 0.05:
            verdict = Verdict(True, f"ensemble {ll_ens:.4f} vs market {ll_mkt:.4f}, "
                                    f"p={p_val:.4f} on n={len(ysub)}", table)
        else:
            verdict = Verdict(False, f"ensemble {ll_ens:.4f} vs market {ll_mkt:.4f}, "
                                     f"p={p_val:.4f} on n={len(ysub)} — not significantly better",
                              table)

    VERDICT_PATH.parent.mkdir(parents=True, exist_ok=True)
    VERDICT_PATH.write_text(("PASS" if verdict.passed else "FAIL") + "\n" + verdict.reason)
    return verdict


def verdict_passed() -> bool:
    """The persisted gate consulted by the value engine."""
    if not VERDICT_PATH.exists():
        return False
    return VERDICT_PATH.read_text().startswith("PASS")
