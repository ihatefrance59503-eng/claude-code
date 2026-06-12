import numpy as np
import pandas as pd
import pytest

from wc26fp.elo import (BASE_RATING, HOME_ADVANTAGE, compute_elo,
                        expected_score, goal_diff_multiplier)


def _df(rows):
    df = pd.DataFrame(rows, columns=[
        "date", "home_team", "away_team", "home_score", "away_score",
        "neutral", "tier"])
    df["date"] = pd.to_datetime(df["date"])
    return df


def test_expected_score_symmetry():
    assert expected_score(1500, 1500) == pytest.approx(0.5)
    assert expected_score(1600, 1500) + expected_score(1500, 1600) == pytest.approx(1.0)


def test_expected_score_400_points_is_10x():
    # the Elo definition: +400 points => 10:1 expected odds
    assert expected_score(1900, 1500) == pytest.approx(10 / 11)


def test_home_edge_applied_only_when_not_neutral():
    df = _df([("2020-01-01", "A", "B", 1, 0, False, 1.0)])
    out = compute_elo(df)
    # home won as slight favourite (home edge) => gains < K/2
    gain = out["elo_home_post"][0] - out["elo_home_pre"][0]
    exp = expected_score(BASE_RATING, BASE_RATING, HOME_ADVANTAGE)
    assert gain == pytest.approx(20.0 * (1 - exp))


def test_zero_sum():
    df = _df([("2020-01-01", "A", "B", 3, 1, True, 4.0),
              ("2020-02-01", "A", "B", 0, 0, True, 1.0)])
    out = compute_elo(df)
    total_change = (out["elo_home_post"] - out["elo_home_pre"]
                    + out["elo_away_post"] - out["elo_away_pre"])
    assert np.allclose(total_change, 0.0)


def test_goal_diff_multiplier_monotonic():
    vals = [goal_diff_multiplier(d) for d in range(1, 7)]
    assert vals == sorted(vals)
    assert goal_diff_multiplier(1) == 1.0
    assert goal_diff_multiplier(2) == 1.5
    assert goal_diff_multiplier(3) == pytest.approx(1.75)


def test_k_scales_with_tier():
    wc = _df([("2020-01-01", "A", "B", 1, 0, True, 4.0)])
    fr = _df([("2020-01-01", "A", "B", 1, 0, True, 1.0)])
    gain_wc = compute_elo(wc)["elo_home_post"][0] - BASE_RATING
    gain_fr = compute_elo(fr)["elo_home_post"][0] - BASE_RATING
    assert gain_wc == pytest.approx(3 * gain_fr)  # K 60 vs 20


def test_pre_ratings_do_not_see_current_match():
    df = _df([("2020-01-01", "A", "B", 5, 0, True, 4.0),
              ("2020-02-01", "A", "C", 1, 1, True, 1.0)])
    out = compute_elo(df)
    assert out["elo_home_pre"][0] == BASE_RATING       # first ever match
    assert out["elo_home_pre"][1] == out["elo_home_post"][0]  # carries forward
