"""Dixon-Coles (1997) bivariate Poisson goals model with time decay.

Gives a full correct-score distribution per match, from which we derive
1X2, Over/Under 1.5/2.5/3.5, BTTS, and clean-sheet prices.
"""
from __future__ import annotations

import numpy as np
import pandas as pd
from scipy.optimize import minimize
from scipy.stats import poisson

MAX_GOALS = 10          # score grid is (0..MAX_GOALS)^2
XI = 0.0018             # time-decay per day (~half-life 1 year)
MIN_MATCHES = 8         # teams below this use a shrunk league-average rating


def _tau(x: np.ndarray, y: np.ndarray, lam: np.ndarray, mu: np.ndarray,
         rho: float) -> np.ndarray:
    """Low-score dependence correction."""
    t = np.ones_like(lam)
    t = np.where((x == 0) & (y == 0), 1 - lam * mu * rho, t)
    t = np.where((x == 0) & (y == 1), 1 + lam * rho, t)
    t = np.where((x == 1) & (y == 0), 1 + mu * rho, t)
    t = np.where((x == 1) & (y == 1), 1 - rho, t)
    return np.clip(t, 1e-10, None)


class DixonColes:
    """Fit attack/defence strengths + home advantage + rho on recent history."""

    def __init__(self, xi: float = XI, max_goals: int = MAX_GOALS) -> None:
        self.xi = xi
        self.max_goals = max_goals
        self.teams_: list[str] = []
        self.attack_: dict[str, float] = {}
        self.defence_: dict[str, float] = {}
        self.home_adv_: float = 0.2
        self.rho_: float = -0.05

    def fit(self, df: pd.DataFrame, ref_date: pd.Timestamp | None = None) -> "DixonColes":
        """df needs: date, home_team, away_team, home_score, away_score, neutral."""
        ref = ref_date or df["date"].max()
        counts = pd.concat([df["home_team"], df["away_team"]]).value_counts()
        keep = counts[counts >= MIN_MATCHES].index
        d = df[df["home_team"].isin(keep) & df["away_team"].isin(keep)].copy()
        self.teams_ = sorted(set(d["home_team"]) | set(d["away_team"]))
        tidx = {t: i for i, t in enumerate(self.teams_)}
        n = len(self.teams_)

        hi = d["home_team"].map(tidx).to_numpy()
        ai = d["away_team"].map(tidx).to_numpy()
        hg = d["home_score"].to_numpy(dtype=float)
        ag = d["away_score"].to_numpy(dtype=float)
        neutral = d["neutral"].to_numpy(dtype=float)
        w = np.exp(-self.xi * (ref - d["date"]).dt.days.to_numpy())

        def nll(params: np.ndarray) -> float:
            atk = params[:n]
            dfc = params[n:2 * n]
            home_adv, rho = params[-2], params[-1]
            lam = np.exp(atk[hi] + dfc[ai] + home_adv * (1 - neutral))
            mu = np.exp(atk[ai] + dfc[hi])
            ll = (
                np.log(_tau(hg, ag, lam, mu, rho))
                + poisson.logpmf(hg, lam) + poisson.logpmf(ag, mu)
            )
            return -float(np.sum(w * ll))

        x0 = np.concatenate([np.zeros(n), np.zeros(n), [0.2, -0.05]])
        # identifiability: mean attack pinned to 0 via penalty
        def nll_pen(params: np.ndarray) -> float:
            return nll(params) + 1000.0 * params[:n].mean() ** 2

        res = minimize(nll_pen, x0, method="L-BFGS-B",
                       options={"maxiter": 200, "maxfun": 100_000})
        atk, dfc = res.x[:n], res.x[n:2 * n]
        self.attack_ = dict(zip(self.teams_, atk))
        self.defence_ = dict(zip(self.teams_, dfc))
        self.home_adv_ = float(res.x[-2])
        self.rho_ = float(np.clip(res.x[-1], -0.9, 0.9))
        return self

    # ------------------------------------------------------------------ #

    def score_grid(self, home: str, away: str, neutral: bool = True) -> np.ndarray:
        """P(home=i, away=j) matrix, normalised to sum to 1."""
        mean_atk = float(np.mean(list(self.attack_.values()))) if self.attack_ else 0.0
        mean_dfc = float(np.mean(list(self.defence_.values()))) if self.defence_ else 0.0
        atk_h = self.attack_.get(home, mean_atk - 0.2)
        dfc_h = self.defence_.get(home, mean_dfc + 0.1)
        atk_a = self.attack_.get(away, mean_atk - 0.2)
        dfc_a = self.defence_.get(away, mean_dfc + 0.1)
        lam = np.exp(atk_h + dfc_a + (0.0 if neutral else self.home_adv_))
        mu = np.exp(atk_a + dfc_h)

        g = np.arange(self.max_goals + 1)
        ph = poisson.pmf(g, lam)
        pa = poisson.pmf(g, mu)
        grid = np.outer(ph, pa)
        # rho correction on the 2x2 low-score block
        for x in (0, 1):
            for y in (0, 1):
                grid[x, y] *= _tau(np.array([x]), np.array([y]),
                                   np.array([lam]), np.array([mu]), self.rho_)[0]
        return grid / grid.sum()

    def market_prices(self, home: str, away: str, neutral: bool = True) -> dict:
        grid = self.score_grid(home, away, neutral)
        i, j = np.indices(grid.shape)
        total = i + j
        out = {
            "p_home": float(grid[i > j].sum()),
            "p_draw": float(np.trace(grid)),
            "p_away": float(grid[i < j].sum()),
            "btts_yes": float(grid[(i > 0) & (j > 0)].sum()),
            "cs_home": float(grid[j == 0].sum()),
            "cs_away": float(grid[i == 0].sum()),
        }
        for line in (1.5, 2.5, 3.5):
            out[f"over_{line}"] = float(grid[total > line].sum())
        return out
