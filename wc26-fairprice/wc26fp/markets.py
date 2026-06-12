"""Bookmaker maths: de-vig (proportional & Shin), edges, Kelly staking."""
from __future__ import annotations

import numpy as np


def implied_probs(odds: list[float] | np.ndarray) -> np.ndarray:
    """Raw implied probabilities (contain the overround)."""
    odds = np.asarray(odds, dtype=float)
    if np.any(odds <= 1.0):
        raise ValueError("decimal odds must be > 1.0")
    return 1.0 / odds


def devig_proportional(odds: list[float] | np.ndarray) -> np.ndarray:
    """Normalise implied probabilities to sum to 1."""
    p = implied_probs(odds)
    return p / p.sum()


def devig_shin(odds: list[float] | np.ndarray, tol: float = 1e-10,
               max_iter: int = 1000) -> np.ndarray:
    """Shin (1992) de-vig: models insider trading proportion z.

    Fixed-point iteration on z; reduces favourite-longshot bias relative to
    proportional normalisation.
    """
    pi = implied_probs(odds)
    beta = float(pi.sum())
    n = len(pi)
    if beta <= 1.0 + 1e-12 or n < 3:  # no overround / two-way: proportional
        return pi / beta
    z = 0.01
    for _ in range(max_iter):
        root = np.sqrt(z * z + 4.0 * (1.0 - z) * (pi * pi) / beta)
        z_next = (float(root.sum()) - 2.0) / (n - 2.0)
        if abs(z_next - z) < tol:
            z = z_next
            break
        z = float(np.clip(z_next, 0.0, 0.5))
    root = np.sqrt(z * z + 4.0 * (1.0 - z) * (pi * pi) / beta)
    p = (root - z) / (2.0 * (1.0 - z))
    return p / p.sum()


def edge(model_prob: float, offered_odds: float) -> float:
    """Expected-value edge in probability points: p_model - p_implied."""
    return model_prob - 1.0 / offered_odds


def kelly_stake(model_prob: float, offered_odds: float, bankroll: float,
                fraction: float = 0.25, cap_frac: float = 0.02) -> float:
    """Fractional Kelly with a hard cap.

    Full Kelly: f* = (b*p - q) / b where b = odds - 1.
    Returns the stake in bankroll units (0 when there is no positive edge).
    """
    b = offered_odds - 1.0
    if b <= 0:
        return 0.0
    q = 1.0 - model_prob
    f_star = (b * model_prob - q) / b
    if f_star <= 0:
        return 0.0
    f = min(f_star * fraction, cap_frac)
    return round(bankroll * f, 2)
