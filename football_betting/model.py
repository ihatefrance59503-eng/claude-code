"""
Logistic regression models for World Cup 2026 betting predictions.

Training data:
  - 50+ real historical WC/Euro/Copa match records (labeled patterns)
  - 15,000 synthetic matches generated from those real distributions
    with team-specific features (xG, fouls, form, FIFA ranking, etc.)

Five separate logistic regression pipelines, one per betting market:
  1. Match result   (home win / draw / away win)
  2. Over 2.5 goals
  3. Both Teams To Score (BTTS)
  4. Total fouls > 25.5
  5. Total cards > 3.5
"""
import numpy as np
from sklearn.linear_model import LogisticRegression
from sklearn.preprocessing import StandardScaler
from sklearn.pipeline import Pipeline
from sklearn.model_selection import cross_val_score
import warnings
warnings.filterwarnings("ignore")

from data_fetcher import TEAM_STATS, DEFAULT_TEAM_STATS, get_h2h_stats
from historical_data import WORLD_CUP_MATCH_PATTERNS, TIER_PROFILES, get_team_tier

# ─────────────────────────────────────────────────────────────────────────────
# Feature extraction
# ─────────────────────────────────────────────────────────────────────────────

def _team_features(team: str) -> list[float]:
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
        s["fifa_ranking"] / 50.0,        # normalise into 0-1 ish range
        s["world_cup_wins"] / 5.0,
        sum(form) / len(form),           # win rate over last 5
        sum(form[-3:]) / 3,              # last 3 games form
    ]


def _match_features(home: str, away: str) -> list[float]:
    hf = _team_features(home)
    af = _team_features(away)
    h2h = get_h2h_stats(home, away)
    total_h2h = max(1, h2h["team_a_wins"] + h2h["draws"] + h2h["team_b_wins"])
    h2h_win_rate = h2h["team_a_wins"] / total_h2h
    h2h_draw_rate = h2h["draws"] / total_h2h

    hs = TEAM_STATS.get(home, DEFAULT_TEAM_STATS)
    aws = TEAM_STATS.get(away, DEFAULT_TEAM_STATS)
    rank_diff = (aws["fifa_ranking"] - hs["fifa_ranking"]) / 50.0  # positive = home ranked better

    diff = [h - a for h, a in zip(hf, af)]
    return hf + af + diff + [
        h2h_win_rate,
        h2h_draw_rate,
        h2h["team_a_goals"],
        h2h["team_b_goals"],
        rank_diff,
    ]


# ─────────────────────────────────────────────────────────────────────────────
# Training data
# ─────────────────────────────────────────────────────────────────────────────

RNG = np.random.default_rng(2026)

_TEAMS = list(TEAM_STATS.keys())


def _tier_stats(tier: str) -> dict:
    return TIER_PROFILES[tier]


def _simulate_features_from_real_pattern(pattern: tuple) -> dict:
    """
    Convert a real historical match pattern into full feature vectors.
    pattern = (hg, ag, hxg, axg, hsot, asot, hf, af, hy, ay, hr, ar, hfr, afr, weight)
    """
    hg, ag, hxg, axg, hsot, asot, hf, af, hy, ay, hr, ar, hfr, afr, _ = pattern
    return {
        "home_goals": hg, "away_goals": ag,
        "home_feats": [hg, ag - hg + 1, hxg, axg, hsot, hf, hy, 0.05, 52 + hfr*10, hr, 0, hfr, hfr],
        "away_feats": [ag, hg - ag + 1, axg, hxg, asot, af, ay, 0.05, 52 + afr*10, ar, 0, afr, afr],
        "total_fouls": hf + af,
        "total_yellows": hy + ay,
    }


def _generate_training_data(n_synthetic: int = 15000):
    """
    Build training arrays by combining:
      1. Real historical patterns (each pattern replicated with noise × weight)
      2. Synthetic matches generated from team-specific distributions
    """
    X_result, y_result = [], []
    X_goals, y_o25 = [], []
    X_btts, y_btts = [], []
    X_fouls, y_fo = [], []
    X_cards, y_co = [], []

    # ── 1. Expand real historical patterns ──────────────────────────────────
    for pattern in WORLD_CUP_MATCH_PATTERNS:
        hg, ag, hxg, axg, hsot, asot, hf, af, hy, ay, hr, ar, hfr, afr, weight = pattern
        replications = int(weight * 40)  # weight 2 → 80 copies, 1 → 40 copies
        for _ in range(replications):
            noise = lambda sd: float(RNG.normal(0, sd))
            # Build synthetic-but-real-anchored feature vector
            h_feats = [
                max(0, hxg + noise(0.3)),           # goals/game proxy
                max(0, axg + noise(0.3)),            # conceded proxy
                max(0, hxg + noise(0.25)),           # xg
                max(0, axg + noise(0.25)),           # xg conceded
                max(0, hsot + noise(1.0)),           # shots on target
                max(5, hf + noise(2.0)),             # fouls
                max(0, hy + noise(0.5)),             # yellows
                max(0, 0.06 + noise(0.03)),          # reds
                max(40, 50 + hfr * 15 + noise(3)),  # possession
                max(1, hr + noise(3)) / 50,          # rank norm
                0.3 + noise(0.1),                    # wc wins norm
                max(0, min(1, hfr + noise(0.15))),   # form
                max(0, min(1, hfr + noise(0.2))),    # recent form
            ]
            a_feats = [
                max(0, axg + noise(0.3)),
                max(0, hxg + noise(0.3)),
                max(0, axg + noise(0.25)),
                max(0, hxg + noise(0.25)),
                max(0, asot + noise(1.0)),
                max(5, af + noise(2.0)),
                max(0, ay + noise(0.5)),
                max(0, 0.06 + noise(0.03)),
                max(40, 50 + afr * 15 + noise(3)),
                max(1, ar + noise(3)) / 50,
                0.2 + noise(0.1),
                max(0, min(1, afr + noise(0.15))),
                max(0, min(1, afr + noise(0.2))),
            ]
            diff = [h - a for h, a in zip(h_feats, a_feats)]
            h2h_wr = 0.4 + (hr - ar) * 0.01 + noise(0.1)
            feats = h_feats + a_feats + diff + [
                float(np.clip(h2h_wr, 0.1, 0.9)),
                0.25 + noise(0.05),
                max(0, hxg + noise(0.2)),
                max(0, axg + noise(0.2)),
                max(-1, min(1, (ar - hr) / 50)),
            ]

            # Simulate outcome with noise around real result
            hg_n = max(0, int(RNG.poisson(max(0.5, hxg + noise(0.5)))))
            ag_n = max(0, int(RNG.poisson(max(0.5, axg + noise(0.5)))))
            result = 1 if hg_n > ag_n else (0 if hg_n == ag_n else -1)
            total_goals = hg_n + ag_n
            btts = 1 if hg_n > 0 and ag_n > 0 else 0
            total_fouls = max(10, int(RNG.normal(hf + af, 3)))
            total_cards = max(0, int(RNG.normal(hy + ay, 1.5)))

            X_result.append(feats); y_result.append(result)
            X_goals.append(feats); y_o25.append(1 if total_goals > 2.5 else 0)
            X_btts.append(feats); y_btts.append(btts)
            X_fouls.append(feats); y_fo.append(1 if total_fouls > 25.5 else 0)
            X_cards.append(feats); y_co.append(1 if total_cards > 3.5 else 0)

    # ── 2. Synthetic matches from team stats ────────────────────────────────
    for _ in range(n_synthetic):
        home, away = RNG.choice(_TEAMS, size=2, replace=False)
        hs = TEAM_STATS[home]
        aws = TEAM_STATS[away]

        feats = _match_features(home, away)

        # Result: rank, form, xG advantage all contribute
        rank_adv = (aws["fifa_ranking"] - hs["fifa_ranking"]) / 100
        form_adv = sum(hs["form"]) / len(hs["form"]) - sum(aws["form"]) / len(aws["form"])
        xg_adv = hs["xg_per_game"] - aws["xg_per_game"]
        home_prob = float(np.clip(0.40 + 0.14*rank_adv + 0.10*form_adv + 0.09*xg_adv, 0.12, 0.78))
        draw_prob = float(np.clip(0.26 - 0.06*abs(rank_adv), 0.12, 0.32))
        away_prob = max(0.05, 1 - home_prob - draw_prob)
        total = home_prob + draw_prob + away_prob
        result = RNG.choice([1, 0, -1], p=[home_prob/total, draw_prob/total, away_prob/total])

        home_scored = int(RNG.poisson(max(0.4, hs["xg_per_game"] * (1 + 0.05*rank_adv))))
        away_scored = int(RNG.poisson(max(0.4, aws["xg_per_game"] * (1 - 0.05*rank_adv))))
        total_goals = home_scored + away_scored
        total_fouls = max(10, int(RNG.normal(hs["fouls_per_game"] + aws["fouls_per_game"], 3.5)))
        total_yellows = max(0, int(RNG.normal(hs["yellow_cards_per_game"] + aws["yellow_cards_per_game"], 1.2)))
        btts = 1 if home_scored > 0 and away_scored > 0 else 0

        X_result.append(feats); y_result.append(result)
        X_goals.append(feats); y_o25.append(1 if total_goals > 2.5 else 0)
        X_btts.append(feats); y_btts.append(btts)
        X_fouls.append(feats); y_fo.append(1 if total_fouls > 25.5 else 0)
        X_cards.append(feats); y_co.append(1 if total_yellows > 3.5 else 0)

    return (
        np.array(X_result), np.array(y_result),
        np.array(X_goals), np.array(y_o25),
        np.array(X_btts), np.array(y_btts),
        np.array(X_fouls), np.array(y_fo),
        np.array(X_cards), np.array(y_co),
    )


# ─────────────────────────────────────────────────────────────────────────────
# Model building
# ─────────────────────────────────────────────────────────────────────────────

def _build_lr() -> Pipeline:
    return Pipeline([
        ("scaler", StandardScaler()),
        ("lr", LogisticRegression(
            max_iter=2000,
            C=0.8,
            solver="lbfgs",
        )),
    ])


def _build_lr_binary() -> Pipeline:
    return Pipeline([
        ("scaler", StandardScaler()),
        ("lr", LogisticRegression(max_iter=2000, C=0.8, solver="lbfgs")),
    ])


print("Training logistic regression models on historical + synthetic data...")
(
    X_res, y_res,
    X_goals, y_o25,
    X_btts, y_btts,
    X_fouls, y_fo,
    X_cards, y_co,
) = _generate_training_data(15000)

print(f"  Training samples: {len(X_res):,}")

_model_result = _build_lr()
_model_result.fit(X_res, y_res)
_cv_result = cross_val_score(_model_result, X_res, y_res, cv=5, scoring="accuracy").mean()

_model_over25 = _build_lr_binary()
_model_over25.fit(X_goals, y_o25)
_cv_over25 = cross_val_score(_model_over25, X_goals, y_o25, cv=5, scoring="accuracy").mean()

_model_btts = _build_lr_binary()
_model_btts.fit(X_btts, y_btts)
_cv_btts = cross_val_score(_model_btts, X_btts, y_btts, cv=5, scoring="accuracy").mean()

_model_fouls = _build_lr_binary()
_model_fouls.fit(X_fouls, y_fo)
_cv_fouls = cross_val_score(_model_fouls, X_fouls, y_fo, cv=5, scoring="accuracy").mean()

_model_cards = _build_lr_binary()
_model_cards.fit(X_cards, y_co)
_cv_cards = cross_val_score(_model_cards, X_cards, y_co, cv=5, scoring="accuracy").mean()

MODEL_ACCURACIES = {
    "Match Result":  round(_cv_result * 100, 1),
    "Over 2.5 Goals": round(_cv_over25 * 100, 1),
    "BTTS":          round(_cv_btts * 100, 1),
    "Fouls":         round(_cv_fouls * 100, 1),
    "Cards":         round(_cv_cards * 100, 1),
}
print(f"  Cross-val accuracies: {MODEL_ACCURACIES}")


# ─────────────────────────────────────────────────────────────────────────────
# Public prediction API
# ─────────────────────────────────────────────────────────────────────────────

def _conf(prob: float) -> str:
    if prob >= 0.68:
        return "HIGH"
    if prob >= 0.54:
        return "MEDIUM"
    return "LOW"


def predict_match(home: str, away: str) -> dict:
    feats = np.array(_match_features(home, away)).reshape(1, -1)

    # 1X2
    result_probs = _model_result.predict_proba(feats)[0]
    classes = list(_model_result.classes_)
    hw = result_probs[classes.index(1)]
    dp = result_probs[classes.index(0)]
    aw = result_probs[classes.index(-1)]

    # Goals
    o25p = _model_over25.predict_proba(feats)[0][1]

    # BTTS
    bttsp = _model_btts.predict_proba(feats)[0][1]

    # Fouls
    fop = _model_fouls.predict_proba(feats)[0][1]

    # Cards
    cop = _model_cards.predict_proba(feats)[0][1]

    hs = TEAM_STATS.get(home, DEFAULT_TEAM_STATS)
    aws = TEAM_STATS.get(away, DEFAULT_TEAM_STATS)
    exp_goals = round(hs["xg_per_game"] + aws["xg_per_game"], 2)
    exp_fouls = round(hs["fouls_per_game"] + aws["fouls_per_game"], 1)

    best_result_prob = max(hw, dp, aw)
    best_result = (f"{home} Win" if best_result_prob == hw
                   else "Draw" if best_result_prob == dp
                   else f"{away} Win")

    return {
        "home": home, "away": away,
        "home_win_prob":  round(hw, 3),
        "draw_prob":      round(dp, 3),
        "away_win_prob":  round(aw, 3),
        "best_result_bet": best_result,
        "result_confidence": _conf(best_result_prob),
        "over25_prob":   round(o25p, 3),
        "under25_prob":  round(1 - o25p, 3),
        "goals_bet":     "Over 2.5" if o25p > 0.5 else "Under 2.5",
        "goals_confidence": _conf(max(o25p, 1 - o25p)),
        "expected_goals": exp_goals,
        "btts_yes_prob": round(bttsp, 3),
        "btts_no_prob":  round(1 - bttsp, 3),
        "btts_bet":      "BTTS Yes" if bttsp > 0.5 else "BTTS No",
        "btts_confidence": _conf(max(bttsp, 1 - bttsp)),
        "fouls_over_prob": round(fop, 3),
        "fouls_bet":     "Fouls Over 25.5" if fop > 0.5 else "Fouls Under 25.5",
        "fouls_confidence": _conf(max(fop, 1 - fop)),
        "expected_fouls": exp_fouls,
        "cards_over_prob": round(cop, 3),
        "cards_bet":     "Cards Over 3.5" if cop > 0.5 else "Cards Under 3.5",
        "cards_confidence": _conf(max(cop, 1 - cop)),
        "model_accuracies": MODEL_ACCURACIES,
        "training_samples": len(X_res),
        "top_picks": _top_picks(home, away, best_result, best_result_prob,
                                o25p, bttsp, fop, cop),
    }


def _top_picks(home, away, result_bet, result_prob,
               o25p, bttsp, fp, cp) -> list[dict]:
    candidates = [
        {"bet": result_bet,
         "prob": result_prob, "market": "Match Result"},
        {"bet": "Over 2.5 Goals" if o25p > 0.5 else "Under 2.5 Goals",
         "prob": max(o25p, 1 - o25p), "market": "Goals"},
        {"bet": "BTTS Yes" if bttsp > 0.5 else "BTTS No",
         "prob": max(bttsp, 1 - bttsp), "market": "BTTS"},
        {"bet": "Fouls Over 25.5" if fp > 0.5 else "Fouls Under 25.5",
         "prob": max(fp, 1 - fp), "market": "Fouls"},
        {"bet": "Cards Over 3.5" if cp > 0.5 else "Cards Under 3.5",
         "prob": max(cp, 1 - cp), "market": "Cards"},
    ]
    picks = sorted(candidates, key=lambda x: x["prob"], reverse=True)
    for p in picks:
        p["confidence"] = _conf(p["prob"])
        p["prob_pct"] = f"{p['prob']*100:.1f}%"
    return [p for p in picks if p["prob"] >= 0.54]
