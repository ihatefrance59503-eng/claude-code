"""
Logistic regression models for World Cup 2026 betting predictions.

Models trained on synthetic historical international match data that
mirrors real-world distributions. Features derived from team stats
fetched by data_fetcher.py.
"""
import numpy as np
import pandas as pd
from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import StandardScaler
from sklearn.pipeline import Pipeline
import warnings
warnings.filterwarnings("ignore")

from data_fetcher import TEAM_STATS, DEFAULT_TEAM_STATS, get_h2h_stats


# ---------------------------------------------------------------------------
# Training data generation
# Mirrors real international match distributions based on historical records.
# In production, replace with scraped historical match data.
# ---------------------------------------------------------------------------

RNG = np.random.default_rng(42)


def _team_feature_vector(team: str) -> list[float]:
    s = TEAM_STATS.get(team, DEFAULT_TEAM_STATS)
    form = s["form"]
    return [
        s["goals_per_game"],
        s["goals_conceded_per_game"],
        s["xg_per_game"],
        s["xg_conceded_per_game"],
        s["shots_on_target_per_game"],
        s["fouls_per_game"],
        s["yellow_cards_per_game"],
        s["red_cards_per_game"],
        s["possession"],
        s["fifa_ranking"],
        sum(form) / len(form),  # form ratio
    ]


def _match_features(home: str, away: str) -> list[float]:
    hf = _team_feature_vector(home)
    af = _team_feature_vector(away)
    h2h = get_h2h_stats(home, away)
    h2h_win_rate = h2h["team_a_wins"] / max(1, h2h["team_a_wins"] + h2h["draws"] + h2h["team_b_wins"])
    # Difference features (home minus away)
    diff = [h - a for h, a in zip(hf, af)]
    return hf + af + diff + [h2h_win_rate, h2h["team_a_goals"], h2h["team_b_goals"]]


def _generate_training_data(n: int = 2000):
    """Simulate n historical matches to train on."""
    teams = list(TEAM_STATS.keys())
    X_result, y_result = [], []
    X_goals, y_over25 = [], []
    X_btts, y_btts = [], []
    X_fouls, y_fouls_over = [], []
    X_cards, y_cards_over = [], []

    for _ in range(n):
        home, away = RNG.choice(teams, size=2, replace=False)
        hs = TEAM_STATS[home]
        aws = TEAM_STATS[away]

        feats = _match_features(home, away)

        # Simulate result
        rank_adv = (aws["fifa_ranking"] - hs["fifa_ranking"]) / 100
        form_adv = sum(hs["form"]) / len(hs["form"]) - sum(aws["form"]) / len(aws["form"])
        xg_adv = hs["xg_per_game"] - aws["xg_per_game"]
        home_prob = 0.42 + 0.12 * rank_adv + 0.1 * form_adv + 0.08 * xg_adv
        home_prob = float(np.clip(home_prob, 0.15, 0.75))
        draw_prob = 0.26
        away_prob = max(0.05, 1 - home_prob - draw_prob)
        # Normalise
        total = home_prob + draw_prob + away_prob
        probs = [home_prob / total, draw_prob / total, away_prob / total]
        result = RNG.choice([1, 0, -1], p=probs)
        X_result.append(feats)
        y_result.append(result)

        # Simulate goals
        avg_goals = hs["goals_per_game"] + aws["goals_per_game"]
        goals = max(0, int(RNG.normal(avg_goals, 1.1)))
        X_goals.append(feats)
        y_over25.append(1 if goals > 2.5 else 0)

        # BTTS
        home_scored = RNG.poisson(hs["goals_per_game"])
        away_scored = RNG.poisson(aws["goals_per_game"])
        X_btts.append(feats)
        y_btts.append(1 if home_scored > 0 and away_scored > 0 else 0)

        # Fouls over 25.5 total
        avg_fouls = hs["fouls_per_game"] + aws["fouls_per_game"]
        total_fouls = max(0, int(RNG.normal(avg_fouls, 3.5)))
        X_fouls.append(feats)
        y_fouls_over.append(1 if total_fouls > 25.5 else 0)

        # Cards over 3.5 total
        avg_cards = hs["yellow_cards_per_game"] + aws["yellow_cards_per_game"]
        total_cards = max(0, int(RNG.normal(avg_cards, 1.2)))
        X_cards.append(feats)
        y_cards_over.append(1 if total_cards > 3.5 else 0)

    return (
        np.array(X_result), np.array(y_result),
        np.array(X_goals), np.array(y_over25),
        np.array(X_btts), np.array(y_btts),
        np.array(X_fouls), np.array(y_fouls_over),
        np.array(X_cards), np.array(y_cards_over),
    )


def _build_pipeline() -> Pipeline:
    return Pipeline([
        ("scaler", StandardScaler()),
        ("lr", LogisticRegression(max_iter=1000, C=1.0, solver="lbfgs")),
    ])


# ---------------------------------------------------------------------------
# Train all models once at import time
# ---------------------------------------------------------------------------
(
    X_res, y_res,
    X_goals, y_o25,
    X_btts, y_btts,
    X_fouls, y_fo,
    X_cards, y_co,
) = _generate_training_data(2000)

_model_result = _build_pipeline()
_model_result.fit(X_res, y_res)

_model_over25 = _build_pipeline()
_model_over25.fit(X_goals, y_o25)

_model_btts = _build_pipeline()
_model_btts.fit(X_btts, y_btts)

_model_fouls = _build_pipeline()
_model_fouls.fit(X_fouls, y_fo)

_model_cards = _build_pipeline()
_model_cards.fit(X_cards, y_co)


# ---------------------------------------------------------------------------
# Public prediction API
# ---------------------------------------------------------------------------

def _confidence_label(prob: float) -> str:
    if prob >= 0.70:
        return "HIGH"
    if prob >= 0.55:
        return "MEDIUM"
    return "LOW"


def _value_rating(prob: float, implied_odds: float = 0.5) -> str:
    edge = prob - implied_odds
    if edge > 0.10:
        return "STRONG VALUE"
    if edge > 0.04:
        return "SLIGHT VALUE"
    return "AVOID"


def predict_match(home: str, away: str) -> dict:
    """
    Run all logistic regression models and return predictions for a match.
    Returns probabilities and recommended bets.
    """
    feats = np.array(_match_features(home, away)).reshape(1, -1)

    # 1X2 result
    result_probs = _model_result.predict_proba(feats)[0]
    classes = list(_model_result.classes_)
    home_win_prob = result_probs[classes.index(1)]
    draw_prob = result_probs[classes.index(0)]
    away_win_prob = result_probs[classes.index(-1)]

    # Over/Under 2.5
    o25_probs = _model_over25.predict_proba(feats)[0]
    over25_prob = o25_probs[1]

    # BTTS
    btts_probs = _model_btts.predict_proba(feats)[0]
    btts_prob = btts_probs[1]

    # Fouls over 25.5
    fo_probs = _model_fouls.predict_proba(feats)[0]
    fouls_over_prob = fo_probs[1]

    # Cards over 3.5
    co_probs = _model_cards.predict_proba(feats)[0]
    cards_over_prob = co_probs[1]

    # Best result bet
    best_result_prob = max(home_win_prob, draw_prob, away_win_prob)
    if best_result_prob == home_win_prob:
        best_result = f"{home} Win"
    elif best_result_prob == draw_prob:
        best_result = "Draw"
    else:
        best_result = f"{away} Win"

    hs = TEAM_STATS.get(home, DEFAULT_TEAM_STATS)
    aws = TEAM_STATS.get(away, DEFAULT_TEAM_STATS)
    expected_goals = round(hs["xg_per_game"] + aws["xg_per_game"], 2)
    expected_fouls = round(hs["fouls_per_game"] + aws["fouls_per_game"], 1)

    return {
        "home": home,
        "away": away,
        # 1X2
        "home_win_prob": round(home_win_prob, 3),
        "draw_prob": round(draw_prob, 3),
        "away_win_prob": round(away_win_prob, 3),
        "best_result_bet": best_result,
        "result_confidence": _confidence_label(best_result_prob),
        "result_value": _value_rating(best_result_prob),
        # Over/Under goals
        "over25_prob": round(over25_prob, 3),
        "under25_prob": round(1 - over25_prob, 3),
        "goals_bet": "Over 2.5" if over25_prob > 0.5 else "Under 2.5",
        "goals_confidence": _confidence_label(max(over25_prob, 1 - over25_prob)),
        "expected_goals": expected_goals,
        # BTTS
        "btts_yes_prob": round(btts_prob, 3),
        "btts_no_prob": round(1 - btts_prob, 3),
        "btts_bet": "BTTS Yes" if btts_prob > 0.5 else "BTTS No",
        "btts_confidence": _confidence_label(max(btts_prob, 1 - btts_prob)),
        # Fouls
        "fouls_over_prob": round(fouls_over_prob, 3),
        "fouls_bet": "Fouls Over 25.5" if fouls_over_prob > 0.5 else "Fouls Under 25.5",
        "fouls_confidence": _confidence_label(max(fouls_over_prob, 1 - fouls_over_prob)),
        "expected_fouls": expected_fouls,
        # Cards
        "cards_over_prob": round(cards_over_prob, 3),
        "cards_bet": "Cards Over 3.5" if cards_over_prob > 0.5 else "Cards Under 3.5",
        "cards_confidence": _confidence_label(max(cards_over_prob, 1 - cards_over_prob)),
        # Summary picks
        "top_picks": _top_picks(
            home, away, best_result, best_result_prob,
            over25_prob, btts_prob, fouls_over_prob, cards_over_prob,
        ),
    }


def _top_picks(home, away, result_bet, result_prob,
               o25_prob, btts_prob, fouls_prob, cards_prob) -> list[dict]:
    candidates = [
        {"bet": result_bet, "prob": result_prob, "market": "Match Result"},
        {"bet": "Over 2.5 Goals" if o25_prob > 0.5 else "Under 2.5 Goals",
         "prob": max(o25_prob, 1 - o25_prob), "market": "Goals"},
        {"bet": "BTTS Yes" if btts_prob > 0.5 else "BTTS No",
         "prob": max(btts_prob, 1 - btts_prob), "market": "BTTS"},
        {"bet": "Fouls Over 25.5" if fouls_prob > 0.5 else "Fouls Under 25.5",
         "prob": max(fouls_prob, 1 - fouls_prob), "market": "Fouls"},
        {"bet": "Cards Over 3.5" if cards_prob > 0.5 else "Cards Under 3.5",
         "prob": max(cards_prob, 1 - cards_prob), "market": "Cards"},
    ]
    # Sort by confidence, only keep HIGH/MEDIUM
    picks = sorted(candidates, key=lambda x: x["prob"], reverse=True)
    for p in picks:
        p["confidence"] = _confidence_label(p["prob"])
        p["prob_pct"] = f"{p['prob']*100:.1f}%"
    return [p for p in picks if p["prob"] >= 0.55]
