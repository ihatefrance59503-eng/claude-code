"""Walk-forward backtest against historical odds.

Requires an odds file (data/cache/intl_odds.csv — see data.load_historical_odds).
Reports ROI with a bootstrap 95% CI, max drawdown, and bet count for both
flat staking and quarter-Kelly. If the CI includes zero the report says,
in so many words, that the strategy is indistinguishable from break-even.
"""
from __future__ import annotations

import numpy as np
import pandas as pd

from .config import CONFIG, SEED
from .features import LABEL, feature_matrix
from .markets import devig_shin, kelly_stake
from .models import Ensemble, time_splits


def run_backtest(ens: Ensemble, df_feats: pd.DataFrame, odds: pd.DataFrame,
                 bankroll: float = 1000.0) -> dict:
    _, _, test = time_splits(df_feats)
    merged = test.merge(odds, on=["date", "home_team", "away_team"], how="inner")
    if merged.empty:
        return {"error": "no test matches have odds — provide data/cache/intl_odds.csv"}

    X = feature_matrix(merged)
    probs = ens.predict_proba(X)
    vc = CONFIG.value

    bets = []
    for i, row in enumerate(merged.itertuples()):
        odds_row = [row.odds_home, row.odds_draw, row.odds_away]
        result = int(row.result)  # 0 home, 1 draw, 2 away
        for sel in range(3):
            p, o = probs[i, sel], odds_row[sel]
            if p - 1.0 / o >= vc.min_edge:
                k_stake = kelly_stake(p, o, bankroll, vc.kelly_fraction,
                                      vc.max_stake_frac)
                won = (sel == result)
                bets.append({
                    "date": row.date, "sel": sel, "odds": o, "prob": p,
                    "flat_pnl": (o - 1) * 10 if won else -10.0,
                    "kelly_stake": k_stake,
                    "kelly_pnl": (o - 1) * k_stake if won else -k_stake,
                })

    if not bets:
        return {"error": "edge threshold produced zero bets on the test window "
                         "(an honest outcome — 'no bet' is a successful output)"}

    bdf = pd.DataFrame(bets).sort_values("date")
    out = {"n_bets": len(bdf)}
    rng = np.random.default_rng(SEED)
    for scheme, pnl_col, staked in (
        ("flat", "flat_pnl", 10.0 * len(bdf)),
        ("quarter_kelly", "kelly_pnl", float(bdf["kelly_stake"].sum())),
    ):
        pnl = bdf[pnl_col].to_numpy()
        roi = pnl.sum() / staked if staked > 0 else 0.0
        idx = rng.integers(0, len(pnl), size=(2000, len(pnl)))
        boot_roi = pnl[idx].sum(axis=1) / staked if staked > 0 else np.zeros(2000)
        lo, hi = np.percentile(boot_roi, [2.5, 97.5])
        equity = pnl.cumsum()
        peak = np.maximum.accumulate(np.concatenate([[0.0], equity]))[1:]
        max_dd = float((peak - equity).max())
        out[scheme] = {
            "roi_pct": round(roi * 100, 2),
            "roi_ci95_pct": [round(lo * 100, 2), round(hi * 100, 2)],
            "max_drawdown": round(max_dd, 2),
            "total_pnl": round(float(pnl.sum()), 2),
            "breakeven_indistinguishable": bool(lo <= 0 <= hi),
        }
    if out["flat"]["breakeven_indistinguishable"]:
        out["note"] = ("ROI confidence interval includes zero: this strategy is "
                       "statistically indistinguishable from break-even.")
    return out
