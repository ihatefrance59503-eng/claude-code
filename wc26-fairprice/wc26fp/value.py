"""Prediction + value engine + bet logging + CLV scoring.

The value engine consults the persisted §4 verdict: when it is FAIL the
engine refuses to recommend stakes (prints fair prices only) unless
override=True is passed explicitly.
"""
from __future__ import annotations

import csv
from datetime import datetime, timezone
from pathlib import Path

import numpy as np
import pandas as pd

from .config import BETS_LOG, CONFIG
from .data import load_results
from .dixon_coles import DixonColes
from .evaluate import verdict_passed
from .features import build_features, feature_matrix
from .markets import devig_shin, edge, kelly_stake
from .models import Ensemble, load_ensemble
from .wc26 import (apply_group_stage_prior, find_match, resolve_team,
                   to_qualify_probs)

BET_COLUMNS = [
    "logged_at", "match_date", "home_team", "away_team", "market", "selection",
    "model_prob", "offered_odds", "market_prob", "edge", "stake", "bankroll",
    "verdict_state", "closing_odds", "result", "pnl",
]


def fair_odds(p: float) -> float:
    return round(1.0 / max(p, 1e-9), 2)


DC_PATH = Path(__file__).resolve().parent.parent / "data" / "dc.pkl"


def fit_and_save_dc() -> DixonColes:
    """Fit Dixon-Coles on the last 8 years and persist it (slow: ~2 min)."""
    import pickle
    res = load_results()
    recent = res[res["date"] >= res["date"].max() - pd.Timedelta(days=8 * 365)]
    dc = DixonColes().fit(recent)
    with open(DC_PATH, "wb") as fh:
        pickle.dump(dc, fh)
    return dc


def load_dc() -> DixonColes:
    import pickle
    if DC_PATH.exists():
        with open(DC_PATH, "rb") as fh:
            return pickle.load(fh)
    return fit_and_save_dc()


class Pricer:
    """Combines the 1X2 ensemble with the Dixon-Coles goals model.

    Features for history + every scheduled fixture are built once at init;
    pricing a scheduled match is then a row lookup. Hypothetical pairings
    take the slower synthetic-row path."""

    def __init__(self, ens: Ensemble | None = None,
                 dc: DixonColes | None = None) -> None:
        self.ens = ens or load_ensemble()
        self.dc = dc or load_dc()
        self._history = load_results()
        from .data import load_future_fixtures
        fixtures = load_future_fixtures()
        combined = pd.concat([self._history, fixtures], ignore_index=True)
        combined = combined.sort_values("date", kind="stable").reset_index(drop=True)
        self._feats = build_features(combined)

    def _features_for(self, home: str, away: str, when: str | None,
                      neutral: bool) -> pd.DataFrame:
        # fast path: scheduled fixture already featurised at init
        f = self._feats
        hit = f[(f["home_team"] == home) & (f["away_team"] == away)
                & f["home_score"].isna()]
        if when:
            dated = hit[hit["date"].dt.normalize() == pd.Timestamp(when).normalize()]
            hit = dated if len(dated) else hit
        if len(hit):
            return hit.tail(1)

        # slow path: hypothetical pairing — append a synthetic row
        row = pd.DataFrame([{
            "date": pd.Timestamp(when) if when
            else self._history["date"].max() + pd.Timedelta(days=1),
            "home_team": home, "away_team": away,
            "home_score": np.nan, "away_score": np.nan,
            "tournament": "FIFA World Cup", "city": "", "country": "",
            "neutral": neutral, "tier": 4.0, "altitude": 0.0,
            "host": (not neutral),
        }])
        df = pd.concat([self._history, row], ignore_index=True)
        return build_features(df).tail(1)

    def price(self, home: str, away: str, when: str | None = None,
              neutral: bool = True, stage: str = "group",
              dead_rubber: bool = False) -> dict:
        home, away = resolve_team(home), resolve_team(away)
        frow = self._features_for(home, away, when, neutral)
        p1x2 = self.ens.predict_proba(feature_matrix(frow))[0]
        if stage == "group":
            p1x2 = apply_group_stage_prior(p1x2, dead_rubber)

        goals = self.dc.market_prices(home, away, neutral)
        out = {
            "home": home, "away": away, "stage": stage,
            "p_home": float(p1x2[0]), "p_draw": float(p1x2[1]), "p_away": float(p1x2[2]),
            "fair_home": fair_odds(p1x2[0]), "fair_draw": fair_odds(p1x2[1]),
            "fair_away": fair_odds(p1x2[2]),
            "goals_model": {k: round(v, 4) for k, v in goals.items()},
            "fair_over25": fair_odds(goals["over_2.5"]),
            "fair_under25": fair_odds(1 - goals["over_2.5"]),
            "fair_btts_yes": fair_odds(goals["btts_yes"]),
        }
        if stage == "knockout":
            elo_h = float(frow["elo_home"].iloc[0])
            elo_a = float(frow["elo_away"].iloc[0])
            out["to_qualify"] = {k: round(v, 4) for k, v in
                                 to_qualify_probs(p1x2, elo_h, elo_a).items()}
        return out

    # ------------------------------------------------------------------ #

    def value(self, home: str, away: str, odds_1x2: list[float],
              bankroll: float = 1000.0, when: str | None = None,
              stage: str | None = None, override: bool = False) -> dict:
        """Compare model fair prices to offered odds; suggest stakes only
        when the integrity gate passed (or override is explicit).

        When the pair matches a scheduled WC26 fixture, venue context
        (host advantage, stage, date) is taken from the fixture."""
        home, away = resolve_team(home), resolve_team(away)
        neutral = True
        fx = find_match(home, away)
        if fx is not None:
            home, away = fx["home_team"], fx["away_team"]
            neutral = fx["home_team"] != fx["country"]
            when = when or str(fx["date"].date())
            stage = stage or fx["stage"]
        pricing = self.price(home, away, when=when, neutral=neutral,
                             stage=stage or "group")
        mkt = devig_shin(odds_1x2)
        gate_ok = verdict_passed() or override

        vc = CONFIG.value
        rows = []
        labels = [("home", pricing["p_home"]), ("draw", pricing["p_draw"]),
                  ("away", pricing["p_away"])]
        for (sel, p_model), o, p_mkt in zip(labels, odds_1x2, mkt):
            e = edge(p_model, o)
            stake = 0.0
            decision = "NO BET"
            if e >= vc.min_edge and gate_ok:
                stake = kelly_stake(p_model, o, bankroll,
                                    vc.kelly_fraction, vc.max_stake_frac)
                decision = f"BET {stake:.2f}" if stake > 0 else "NO BET"
            elif e >= vc.min_edge and not gate_ok:
                decision = "NO BET (verdict gate: FAIL)"
            rows.append({
                "selection": sel, "model_prob": round(p_model, 4),
                "offered_odds": o, "market_prob": round(float(p_mkt), 4),
                "edge_pp": round(e * 100, 2), "stake": stake,
                "decision": decision,
            })
        return {**pricing, "gate_passed": gate_ok, "bankroll": bankroll,
                "assessment": rows}


# --------------------------------------------------------------------------- #
# Bet logging + CLV
# --------------------------------------------------------------------------- #

def log_bet(match_date: str, home: str, away: str, market: str, selection: str,
            model_prob: float, offered_odds: float, market_prob: float,
            stake: float, bankroll: float) -> None:
    new = not Path(BETS_LOG).exists()
    with open(BETS_LOG, "a", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=BET_COLUMNS)
        if new:
            w.writeheader()
        w.writerow({
            "logged_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            "match_date": match_date, "home_team": home, "away_team": away,
            "market": market, "selection": selection,
            "model_prob": round(model_prob, 4), "offered_odds": offered_odds,
            "market_prob": round(market_prob, 4),
            "edge": round(model_prob - 1 / offered_odds, 4),
            "stake": stake, "bankroll": bankroll,
            "verdict_state": "PASS" if verdict_passed() else "FAIL",
            "closing_odds": "", "result": "", "pnl": "",
        })


def clv_report() -> pd.DataFrame | str:
    """Score logged bets against closing lines — the single best predictor
    of whether an edge is real. Fill the closing_odds column in bets.csv
    (manually or via the Odds API just before kickoff)."""
    if not Path(BETS_LOG).exists():
        return "no bets logged yet"
    df = pd.read_csv(BETS_LOG)
    scored = df[pd.to_numeric(df["closing_odds"], errors="coerce").notna()].copy()
    if scored.empty:
        return (f"{len(df)} bets logged, none have closing_odds filled in yet — "
                "add closing prices to data/bets.csv to compute CLV")
    scored["closing_odds"] = scored["closing_odds"].astype(float)
    scored["clv_pct"] = (scored["offered_odds"] / scored["closing_odds"] - 1) * 100
    summary = pd.DataFrame({
        "bets_scored": [len(scored)],
        "avg_clv_pct": [scored["clv_pct"].mean().round(2)],
        "positive_clv_rate": [(scored["clv_pct"] > 0).mean().round(3)],
    })
    return summary
