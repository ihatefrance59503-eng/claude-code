"""Self-computed Elo ratings from the full match history.

- K scales with competition importance (World Cup 60 ... friendly 20)
- goal-difference multiplier (margin-of-victory)
- home advantage offset applied inside the expectation, never to stored ratings
- neutral venues get no offset
- pre-match ratings are persisted per row: features never see the future
"""
from __future__ import annotations

import numpy as np
import pandas as pd

BASE_RATING = 1500.0
HOME_ADVANTAGE = 60.0  # Elo points added to home expectation when not neutral

# K factor by competition tier (see data.TIER)
K_BY_TIER = {4.0: 60.0, 3.0: 50.0, 2.5: 40.0, 2.0: 30.0, 1.0: 20.0}
DEFAULT_K = 30.0


def expected_score(rating_a: float, rating_b: float, home_edge: float = 0.0) -> float:
    """P(A beats B) under the Elo logistic curve, draw counted as half."""
    return 1.0 / (1.0 + 10.0 ** (-((rating_a + home_edge) - rating_b) / 400.0))


def goal_diff_multiplier(goal_diff: int) -> float:
    """Standard World Football Elo margin multiplier."""
    gd = abs(goal_diff)
    if gd <= 1:
        return 1.0
    if gd == 2:
        return 1.5
    return (11.0 + gd) / 8.0  # 3 -> 1.75, 4 -> 1.875, ...


def compute_elo(results: pd.DataFrame) -> pd.DataFrame:
    """Run Elo over the (date-sorted) history.

    Returns a copy of `results` with columns:
      elo_home_pre, elo_away_pre  — ratings BEFORE the match (leakage-safe)
      elo_home_post, elo_away_post
    """
    df = results.sort_values("date", kind="stable").reset_index(drop=True)
    ratings: dict[str, float] = {}
    n = len(df)
    h_pre = np.empty(n)
    a_pre = np.empty(n)
    h_post = np.empty(n)
    a_post = np.empty(n)

    home_arr = df["home_team"].to_numpy()
    away_arr = df["away_team"].to_numpy()
    hs_arr = df["home_score"].to_numpy()
    as_arr = df["away_score"].to_numpy()
    neutral_arr = df["neutral"].to_numpy()
    tier_arr = df["tier"].to_numpy()

    for i in range(n):
        home, away = home_arr[i], away_arr[i]
        rh = ratings.get(home, BASE_RATING)
        ra = ratings.get(away, BASE_RATING)
        h_pre[i], a_pre[i] = rh, ra

        hs, as_ = hs_arr[i], as_arr[i]
        if np.isnan(hs):  # future fixture — carry ratings through unchanged
            h_post[i], a_post[i] = rh, ra
            continue

        home_edge = 0.0 if neutral_arr[i] else HOME_ADVANTAGE
        exp_home = expected_score(rh, ra, home_edge)
        actual = 1.0 if hs > as_ else (0.5 if hs == as_ else 0.0)
        k = K_BY_TIER.get(tier_arr[i], DEFAULT_K)
        delta = k * goal_diff_multiplier(int(hs - as_)) * (actual - exp_home)

        ratings[home] = rh + delta
        ratings[away] = ra - delta
        h_post[i], a_post[i] = ratings[home], ratings[away]

    out = df.copy()
    out["elo_home_pre"] = h_pre
    out["elo_away_pre"] = a_pre
    out["elo_home_post"] = h_post
    out["elo_away_post"] = a_post
    return out


def current_ratings(results: pd.DataFrame) -> pd.Series:
    """Latest rating per team after replaying all history."""
    df = compute_elo(results)
    last: dict[str, float] = {}
    for _, row in df.iterrows():
        last[row["home_team"]] = row["elo_home_post"]
        last[row["away_team"]] = row["elo_away_post"]
    return pd.Series(last).sort_values(ascending=False)
