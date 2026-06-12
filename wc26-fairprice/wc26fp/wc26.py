"""World Cup 2026 mode: fixture calendar, venues, tournament adjustments.

The fixture list ships inside the cached martj42 results.csv (scheduled
matches carry NA scores) so `refresh` keeps it current as the tournament
progresses. Venue coordinates/altitude live in data/wc26_venues.csv.

Tournament adjustments (documented, tunable):
- HOSTS get the home-advantage Elo offset even on technically-neutral rows.
- DRAW_INFLATION: group-stage draw rates at WCs run above qualifier rates;
  we nudge p_draw up by a small prior and renormalise.
- MD3 dead rubbers: when qualification is already settled, rotation risk
  rises; we damp the favourite's edge toward the prior (DEAD_RUBBER_DAMP).
- Knockout "to qualify": P(win in 90') + P(draw in 90') x P(win ET/pens),
  where ET/pens is approximated from Elo diff shrunk toward a coin flip
  (pens are close to 50/50 — documented approximation, not a pens model).
"""
from __future__ import annotations

import math
from datetime import date as date_t

import numpy as np
import pandas as pd

from .config import DATA_DIR
from .data import load_future_fixtures, load_results
from .elo import expected_score

HOSTS = {"United States", "Canada", "Mexico"}
DRAW_INFLATION = 0.02      # absolute bump to p_draw in group stage
DEAD_RUBBER_DAMP = 0.25    # shrink factor toward uniform on MD3 dead rubbers
GROUP_STAGE_END = pd.Timestamp("2026-06-27")


def load_venues() -> pd.DataFrame:
    return pd.read_csv(DATA_DIR / "wc26_venues.csv")


def haversine_km(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    r = 6371.0
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dphi = math.radians(lat2 - lat1)
    dlmb = math.radians(lon2 - lon1)
    a = math.sin(dphi / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dlmb / 2) ** 2
    return 2 * r * math.asin(math.sqrt(a))


def wc26_fixtures() -> pd.DataFrame:
    """All scheduled WC2026 matches (group stage; knockouts appear in the
    source file once pairings are known), enriched with venue facts."""
    fx = load_future_fixtures()
    fx = fx[fx["tournament"] == "FIFA World Cup"].copy()
    venues = load_venues().drop_duplicates("city").set_index("city")
    fx["lat"] = fx["city"].map(venues["lat"])
    fx["lon"] = fx["city"].map(venues["lon"])
    fx["altitude_m"] = fx["city"].map(venues["altitude_m"]).fillna(0)
    fx["stage"] = np.where(fx["date"] <= GROUP_STAGE_END, "group", "knockout")
    return fx.reset_index(drop=True)


def wc26_played() -> pd.DataFrame:
    res = load_results()
    res = res[(res["tournament"] == "FIFA World Cup") & (res["date"] >= "2026-06-01")]
    return res.reset_index(drop=True)


def todays_matches(today: str | None = None) -> pd.DataFrame:
    fx = wc26_fixtures()
    today_ts = pd.Timestamp(today or date_t.today())
    return fx[fx["date"].dt.normalize() == today_ts.normalize()].reset_index(drop=True)


def find_match(team_a: str, team_b: str) -> pd.Series | None:
    """Locate a scheduled WC26 fixture by (fuzzy) team names, either order."""
    fx = wc26_fixtures()
    a, b = team_a.lower(), team_b.lower()

    def match(row: pd.Series) -> bool:
        h, w = row["home_team"].lower(), row["away_team"].lower()
        return ((a in h or h.startswith(a)) and (b in w or w.startswith(b))) or \
               ((b in h or h.startswith(b)) and (a in w or w.startswith(a)))

    hits = fx[fx.apply(match, axis=1)]
    return hits.iloc[0] if len(hits) else None


# Common 3-letter codes -> dataset names (extend as needed)
TEAM_CODES = {
    "USA": "United States", "CAN": "Canada", "MEX": "Mexico",
    "ARG": "Argentina", "BRA": "Brazil", "FRA": "France", "ENG": "England",
    "GER": "Germany", "ESP": "Spain", "POR": "Portugal", "NED": "Netherlands",
    "BEL": "Belgium", "CRO": "Croatia", "ITA": "Italy", "URU": "Uruguay",
    "COL": "Colombia", "ECU": "Ecuador", "PAR": "Paraguay", "BOL": "Bolivia",
    "JPN": "Japan", "KOR": "South Korea", "AUS": "Australia", "IRN": "Iran",
    "KSA": "Saudi Arabia", "QAT": "Qatar", "UZB": "Uzbekistan", "JOR": "Jordan",
    "MAR": "Morocco", "SEN": "Senegal", "GHA": "Ghana", "EGY": "Egypt",
    "ALG": "Algeria", "TUN": "Tunisia", "CIV": "Ivory Coast", "RSA": "South Africa",
    "NGA": "Nigeria", "CMR": "Cameroon", "BIH": "Bosnia and Herzegovina",
    "SUI": "Switzerland", "AUT": "Austria", "SCO": "Scotland", "WAL": "Wales",
    "NOR": "Norway", "DEN": "Denmark", "SWE": "Sweden", "POL": "Poland",
    "CZE": "Czech Republic", "SVK": "Slovakia", "SRB": "Serbia", "TUR": "Turkey",
    "UKR": "Ukraine", "HUN": "Hungary", "ALB": "Albania", "PAN": "Panama",
    "CRC": "Costa Rica", "HON": "Honduras", "JAM": "Jamaica", "NZL": "New Zealand",
    "PER": "Peru", "CHI": "Chile", "VEN": "Venezuela", "HAI": "Haiti",
    "CUW": "Curaçao", "CPV": "Cape Verde",
}


def resolve_team(name_or_code: str) -> str:
    return TEAM_CODES.get(name_or_code.upper(), name_or_code)


# --------------------------------------------------------------------------- #
# Tournament probability adjustments
# --------------------------------------------------------------------------- #

def apply_group_stage_prior(p: np.ndarray, dead_rubber: bool = False) -> np.ndarray:
    """p = [home, draw, away]. Draw inflation + optional dead-rubber damping."""
    p = p.copy().astype(float)
    p[1] += DRAW_INFLATION
    p /= p.sum()
    if dead_rubber:
        uniform = np.array([1 / 3, 1 / 3, 1 / 3])
        p = (1 - DEAD_RUBBER_DAMP) * p + DEAD_RUBBER_DAMP * uniform
        p /= p.sum()
    return p


def to_qualify_probs(p90: np.ndarray, elo_home: float, elo_away: float) -> dict:
    """Knockout advancement prices from 90-minute probabilities.

    ET/pens model: Elo expectation shrunk 60% toward 0.5 (penalties are
    nearly a coin flip; extra time retains a little of the quality gap).
    This is a documented approximation, not a penalties model.
    """
    p_et_home = 0.5 + 0.4 * (expected_score(elo_home, elo_away) - 0.5)
    q_home = float(p90[0] + p90[1] * p_et_home)
    return {"home_qualify": q_home, "away_qualify": 1.0 - q_home,
            "et_pens_home_given_draw": p_et_home}
