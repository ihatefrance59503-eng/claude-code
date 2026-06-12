"""The leakage gate: features for a match must be identical whether or not
any LATER match exists in the dataset. If a feature peeks at the future,
truncating the future changes it and this test fails."""
import numpy as np
import pandas as pd

from wc26fp.features import FEATURES, build_features

# FIFA points join uses an external file keyed by real team names; the
# synthetic teams here get fifa_pts_diff = 0 either way, which is fine —
# every history-derived feature (Elo, form, rest) is exercised.


def _history(n=60, seed=3):
    rng = np.random.default_rng(seed)
    teams = ["Alpha", "Beta", "Gamma", "Delta"]
    rows = []
    for k in range(n):
        h, a = rng.choice(teams, 2, replace=False)
        rows.append({
            "date": pd.Timestamp("2023-01-01") + pd.Timedelta(days=3 * k),
            "home_team": h, "away_team": a,
            "home_score": int(rng.poisson(1.4)), "away_score": int(rng.poisson(1.1)),
            "tournament": "Friendly", "city": "X", "country": "Y",
            "neutral": bool(rng.integers(0, 2)),
            "tier": float(rng.choice([1.0, 2.5, 4.0])),
            "altitude": 0.0, "host": False,
        })
    return pd.DataFrame(rows)


def test_features_immune_to_future_results():
    full = _history()
    cut = 40  # the match under inspection
    feats_full = build_features(full)
    feats_trunc = build_features(full.iloc[: cut + 1].copy())

    row_full = feats_full.iloc[cut][FEATURES].astype(float)
    row_trunc = feats_trunc.iloc[cut][FEATURES].astype(float)
    pd.testing.assert_series_equal(row_full, row_trunc, check_names=False)


def test_first_match_has_neutral_priors():
    df = _history(n=1)
    feats = build_features(df)
    # no history: Elo diff is just the home offset (or 0 on neutral),
    # form diffs are zero-filled
    assert feats.iloc[0]["form5_gf_diff"] == 0.0
    assert feats.iloc[0]["form10_wr_diff"] == 0.0


def test_pre_elo_not_post_elo():
    df = _history(n=10)
    feats = build_features(df)
    # the stored feature must equal the PRE rating, never the post
    assert (feats["elo_home"] == feats["elo_home_pre"]).all()
    changed = feats["elo_home_post"] != feats["elo_home_pre"]
    assert changed.any()  # ratings do move, so pre != post somewhere
