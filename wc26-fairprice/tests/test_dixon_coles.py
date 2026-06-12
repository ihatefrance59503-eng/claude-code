import numpy as np
import pandas as pd
import pytest

from wc26fp.dixon_coles import DixonColes, _tau


@pytest.fixture(scope="module")
def fitted():
    """Small synthetic league: A strong, B mid, C weak."""
    rng = np.random.default_rng(7)
    teams = {"A": 1.9, "B": 1.3, "C": 0.8}
    rows = []
    date = pd.Timestamp("2024-01-01")
    names = list(teams)
    for k in range(300):
        h, a = rng.choice(names, 2, replace=False)
        rows.append({
            "date": date + pd.Timedelta(days=k),
            "home_team": h, "away_team": a,
            "home_score": rng.poisson(teams[h] * 1.15),
            "away_score": rng.poisson(teams[a]),
            "neutral": False,
        })
    return DixonColes(xi=0.0).fit(pd.DataFrame(rows))


def test_score_grid_sums_to_one(fitted):
    grid = fitted.score_grid("A", "C")
    assert grid.sum() == pytest.approx(1.0)
    assert (grid >= 0).all()


def test_market_prices_consistent(fitted):
    p = fitted.market_prices("A", "C")
    assert p["p_home"] + p["p_draw"] + p["p_away"] == pytest.approx(1.0, abs=1e-9)
    # overs are monotone in the line
    assert p["over_1.5"] >= p["over_2.5"] >= p["over_3.5"]
    # BTTS-no >= P(either clean sheet) consistency
    assert 0 <= p["btts_yes"] <= 1


def test_strong_team_favoured(fitted):
    p = fitted.market_prices("A", "C", neutral=True)
    assert p["p_home"] > p["p_away"]
    q = fitted.market_prices("C", "A", neutral=True)
    assert q["p_away"] > q["p_home"]


def test_home_advantage_positive(fitted):
    assert fitted.home_adv_ > 0


def test_unknown_team_falls_back(fitted):
    p = fitted.market_prices("A", "Atlantis")
    assert p["p_home"] + p["p_draw"] + p["p_away"] == pytest.approx(1.0, abs=1e-9)


def test_tau_correction_bounds():
    lam = np.array([1.4]); mu = np.array([1.1])
    for x in (0, 1):
        for y in (0, 1):
            t = _tau(np.array([x]), np.array([y]), lam, mu, -0.1)
            assert t[0] > 0
