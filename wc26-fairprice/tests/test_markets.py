import numpy as np
import pytest

from wc26fp.markets import (devig_proportional, devig_shin, edge,
                            implied_probs, kelly_stake)


def test_implied_probs():
    np.testing.assert_allclose(implied_probs([2.0, 4.0]), [0.5, 0.25])


def test_implied_rejects_bad_odds():
    with pytest.raises(ValueError):
        implied_probs([1.0, 2.0])


def test_proportional_sums_to_one():
    p = devig_proportional([1.83, 3.5, 4.6])
    assert p.sum() == pytest.approx(1.0)
    assert (p > 0).all()


def test_shin_sums_to_one_and_orders_match():
    odds = [1.5, 4.0, 7.0]
    p = devig_shin(odds)
    assert p.sum() == pytest.approx(1.0, abs=1e-8)
    assert p[0] > p[1] > p[2]


def test_shin_compresses_longshots_vs_proportional():
    # Shin should assign LESS probability to the longshot than proportional
    odds = [1.3, 5.0, 12.0]
    prop = devig_proportional(odds)
    shin = devig_shin(odds)
    assert shin[2] < prop[2]
    assert shin[0] > prop[0]


def test_no_overround_passthrough():
    odds = [2.0, 4.0, 4.0]  # implied sums to exactly 1
    np.testing.assert_allclose(devig_shin(odds), [0.5, 0.25, 0.25], atol=1e-9)


def test_edge():
    assert edge(0.5, 2.5) == pytest.approx(0.1)
    assert edge(0.4, 2.5) == pytest.approx(0.0)


def test_kelly_zero_when_no_edge():
    assert kelly_stake(0.4, 2.5, 1000) == 0.0       # fair => f*=0
    assert kelly_stake(0.3, 2.5, 1000) == 0.0       # negative edge


def test_kelly_quarter_and_cap():
    # p=0.5, odds=2.2 => b=1.2, f* = (0.6-0.5)/1.2 = 0.08333
    full = (1.2 * 0.5 - 0.5) / 1.2
    quarter = full * 0.25
    assert kelly_stake(0.5, 2.2, 1000, fraction=0.25, cap_frac=1.0) == \
        pytest.approx(1000 * quarter, abs=0.01)
    # cap binds at 2%
    assert kelly_stake(0.9, 5.0, 1000, fraction=0.25, cap_frac=0.02) == 20.0
