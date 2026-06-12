"""Feature engineering — strictly pre-match information only.

Every feature for match i is computed from rows with date < date_i (or
static venue facts). `tests/test_leakage.py` enforces this by checking that
features for a given match are identical whether or not later results exist.
"""
from __future__ import annotations

import numpy as np
import pandas as pd

from .data import load_fifa_rankings
from .elo import HOME_ADVANTAGE, compute_elo

FEATURES = [
    "elo_diff",            # home - away, including home offset when applicable
    "elo_home", "elo_away",
    "fifa_pts_diff",
    "neutral", "host",
    "tier", "altitude",
    "rest_diff",           # home rest days - away rest days (capped)
    "form5_gf_diff", "form5_ga_diff", "form5_wr_diff",
    "form10_gf_diff", "form10_ga_diff", "form10_wr_diff",
    # squad-value / club-elo / xG slots: NaN-imputed to 0 when source disabled
    "squad_value_diff", "club_elo_diff", "xg_form_diff",
]

LABEL = "result"  # 0 = home win, 1 = draw, 2 = away win (RPS-ordered)


def _team_long(df: pd.DataFrame) -> pd.DataFrame:
    """Stack to one row per (team, match) for rolling-form computation."""
    home = pd.DataFrame({
        "date": df["date"], "team": df["home_team"],
        "gf": df["home_score"], "ga": df["away_score"],
        "win": (df["home_score"] > df["away_score"]).astype(float),
        "tier": df["tier"], "midx": df.index,
    })
    away = pd.DataFrame({
        "date": df["date"], "team": df["away_team"],
        "gf": df["away_score"], "ga": df["home_score"],
        "win": (df["away_score"] > df["home_score"]).astype(float),
        "tier": df["tier"], "midx": df.index,
    })
    long = pd.concat([home, away], ignore_index=True)
    return long.sort_values(["team", "date", "midx"], kind="stable")


def _rolling_form(long: pd.DataFrame, window: int) -> pd.DataFrame:
    """Competitiveness-weighted rolling means over the previous `window`
    matches, shifted so the current match never sees itself."""
    g = long.groupby("team", sort=False)
    w = long["tier"]
    out = pd.DataFrame(index=long.index)
    for col in ("gf", "ga", "win"):
        weighted = long[col] * w
        num = g[col].transform(lambda s: s.shift(1).rolling(window, min_periods=1).mean())
        # weight competitive matches over friendlies: weighted mean
        wnum = weighted.groupby(long["team"]).transform(
            lambda s: s.shift(1).rolling(window, min_periods=1).sum())
        wden = w.groupby(long["team"]).transform(
            lambda s: s.shift(1).rolling(window, min_periods=1).sum())
        out[f"{col}{window}"] = (wnum / wden).fillna(num)
    out["rest_days"] = g["date"].transform(lambda s: (s - s.shift(1)).dt.days)
    return out


def build_features(results: pd.DataFrame) -> pd.DataFrame:
    """Returns results + Elo columns + FEATURES + LABEL.

    Accepts played matches; future fixtures (NaN scores) are allowed and get
    features but a NaN label.
    """
    df = compute_elo(results)

    # --- Elo, with the venue offset folded into the diff -------------------
    edge = np.where(df["neutral"], 0.0, HOME_ADVANTAGE)
    df["elo_home"] = df["elo_home_pre"]
    df["elo_away"] = df["elo_away_pre"]
    df["elo_diff"] = (df["elo_home_pre"] + edge) - df["elo_away_pre"]

    # --- FIFA point totals (merge_asof: latest ranking BEFORE match) -------
    fifa = load_fifa_rankings()
    if len(fifa):
        fifa = fifa.sort_values("date")
        for side in ("home", "away"):
            merged = pd.merge_asof(
                df[["date", f"{side}_team"]].reset_index().sort_values("date"),
                fifa.rename(columns={"team": f"{side}_team"}),
                on="date", by=f"{side}_team", direction="backward",
            ).set_index("index").sort_index()
            df[f"fifa_pts_{side}"] = merged["total_points"]
        df["fifa_pts_diff"] = (df["fifa_pts_home"] - df["fifa_pts_away"]).fillna(0.0)
    else:
        df["fifa_pts_diff"] = 0.0

    # --- Rolling form (5 / 10), weighted competitive > friendly ------------
    long = _team_long(df)
    for window in (5, 10):
        form = _rolling_form(long, window)
        form["team"] = long["team"]
        form["midx"] = long["midx"]
        # map back: one row per (midx, team)
        keyed = form.set_index(["midx", "team"])
        for side in ("home", "away"):
            idx = pd.MultiIndex.from_arrays([df.index, df[f"{side}_team"]])
            picked = keyed.reindex(idx)
            df[f"{side}_gf{window}"] = picked[f"gf{window}"].to_numpy()
            df[f"{side}_ga{window}"] = picked[f"ga{window}"].to_numpy()
            df[f"{side}_wr{window}"] = picked[f"win{window}"].to_numpy()
            if window == 5:
                df[f"{side}_rest"] = picked["rest_days"].to_numpy()
        df[f"form{window}_gf_diff"] = (df[f"home_gf{window}"] - df[f"away_gf{window}"]).fillna(0.0)
        df[f"form{window}_ga_diff"] = (df[f"home_ga{window}"] - df[f"away_ga{window}"]).fillna(0.0)
        df[f"form{window}_wr_diff"] = (df[f"home_wr{window}"] - df[f"away_wr{window}"]).fillna(0.0)

    rest_h = df["home_rest"].clip(upper=30).fillna(14)
    rest_a = df["away_rest"].clip(upper=30).fillna(14)
    df["rest_diff"] = rest_h - rest_a

    # --- Optional-source slots (0 when the source is disabled) -------------
    for col in ("squad_value_diff", "club_elo_diff", "xg_form_diff"):
        if col not in df.columns:
            df[col] = 0.0

    df["neutral"] = df["neutral"].astype(float)
    df["host"] = df["host"].astype(float)

    # --- Label --------------------------------------------------------------
    df[LABEL] = np.select(
        [df["home_score"] > df["away_score"], df["home_score"] == df["away_score"]],
        [0, 1], default=2,
    ).astype(float)
    df.loc[df["home_score"].isna(), LABEL] = np.nan

    return df


def feature_matrix(df: pd.DataFrame) -> pd.DataFrame:
    return df[FEATURES].astype(float).fillna(0.0)
