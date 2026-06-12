"""Fixture-file and tournament-adjustment integrity."""
import numpy as np
import pandas as pd
import pytest

from wc26fp.config import CACHE_DIR, DATA_DIR
from wc26fp.wc26 import (apply_group_stage_prior, haversine_km, load_venues,
                         resolve_team, to_qualify_probs)

needs_data = pytest.mark.skipif(
    not (CACHE_DIR / "results.csv").exists(), reason="run `refresh` first")


def test_venues_file_integrity():
    v = load_venues()
    assert {"city", "lat", "lon", "altitude_m"} <= set(v.columns)
    assert v["lat"].between(-90, 90).all()
    assert v["lon"].between(-180, 180).all()
    assert v["altitude_m"].between(0, 3000).all()
    azteca = v[v["city"] == "Mexico City"].iloc[0]
    assert azteca["altitude_m"] == 2240  # the famous one


def test_haversine_known_distance():
    # Mexico City -> East Rutherford ~ 3,360 km
    d = haversine_km(19.3029, -99.1505, 40.8135, -74.0745)
    assert 3200 < d < 3600
    assert haversine_km(0, 0, 0, 0) == 0.0


@needs_data
def test_wc26_fixture_calendar():
    from wc26fp.wc26 import wc26_fixtures
    fx = wc26_fixtures()
    assert len(fx) > 0
    assert (fx["tournament"] == "FIFA World Cup").all()
    assert fx["date"].min() >= pd.Timestamp("2026-06-01")
    teams = set(fx["home_team"]) | set(fx["away_team"])
    assert len(teams) >= 40  # 48 once all fixtures present in source


def test_team_code_resolution():
    assert resolve_team("CAN") == "Canada"
    assert resolve_team("BIH") == "Bosnia and Herzegovina"
    assert resolve_team("Unknown Team") == "Unknown Team"


def test_group_prior_normalised():
    p = apply_group_stage_prior(np.array([0.5, 0.25, 0.25]))
    assert p.sum() == pytest.approx(1.0)
    assert p[1] > 0.25  # draw inflated


def test_dead_rubber_damps_favourite():
    p0 = np.array([0.7, 0.2, 0.1])
    p = apply_group_stage_prior(p0, dead_rubber=True)
    assert p[0] < p0[0]
    assert p.sum() == pytest.approx(1.0)


def test_to_qualify_bounds_and_symmetry():
    p90 = np.array([0.45, 0.30, 0.25])
    q = to_qualify_probs(p90, 1900.0, 1800.0)
    assert 0 < q["home_qualify"] < 1
    assert q["home_qualify"] + q["away_qualify"] == pytest.approx(1.0)
    assert q["home_qualify"] > 0.45            # draws break favourably
    # equal Elo => ET/pens is a coin flip
    q_eq = to_qualify_probs(p90, 1700.0, 1700.0)
    assert q_eq["et_pens_home_given_draw"] == pytest.approx(0.5)
