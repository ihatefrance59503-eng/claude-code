"""World Cup 2026 Football Betting Assistant - Flask app."""
import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

from flask import Flask, render_template, request, jsonify
from data_fetcher import get_all_teams, get_team, get_upcoming_fixtures, WC2026_GROUPS
from model import predict_match

app = Flask(__name__)


@app.route("/")
def index():
    fixtures = get_upcoming_fixtures()
    # Pre-compute quick predictions for fixtures
    predictions = []
    for f in fixtures:
        pred = predict_match(f["home"], f["away"])
        predictions.append({**f, **pred})
    return render_template("index.html", predictions=predictions, groups=WC2026_GROUPS)


@app.route("/match")
def match():
    home = request.args.get("home", "")
    away = request.args.get("away", "")
    if not home or not away:
        return render_template("match.html", error="Please select both teams.", teams=list(WC2026_GROUPS.values()))

    all_teams = [t for group in WC2026_GROUPS.values() for t in group]
    if home not in all_teams or away not in all_teams:
        return render_template("match.html", error="Unknown team.", teams=all_teams)
    if home == away:
        return render_template("match.html", error="Teams must be different.", teams=all_teams)

    pred = predict_match(home, away)
    home_team = get_team(home)
    away_team = get_team(away)
    return render_template("match.html", pred=pred, home_team=home_team, away_team=away_team,
                           all_teams=all_teams)


@app.route("/teams")
def teams():
    all_teams = get_all_teams()
    return render_template("teams.html", teams=all_teams, groups=WC2026_GROUPS)


@app.route("/team/<name>")
def team_detail(name: str):
    team = get_team(name)
    if not team:
        return render_template("team_detail.html", error=f"Team '{name}' not found.")
    all_teams = [t for group in WC2026_GROUPS.values() for t in group]
    # Generate predictions vs all group opponents
    group = next((g for g, members in WC2026_GROUPS.items() if name in members), None)
    opponents = [t for t in (WC2026_GROUPS.get(group, [])) if t != name]
    match_preds = [predict_match(name, opp) for opp in opponents]
    return render_template("team_detail.html", team=team, group=group,
                           opponents=opponents, match_preds=match_preds)


@app.route("/api/predict")
def api_predict():
    home = request.args.get("home", "")
    away = request.args.get("away", "")
    if not home or not away:
        return jsonify({"error": "Provide home and away query params"}), 400
    try:
        return jsonify(predict_match(home, away))
    except Exception as e:
        return jsonify({"error": str(e)}), 500


if __name__ == "__main__":
    # Bind to all interfaces so phones on the same network can connect,
    # or so a cloudflared tunnel can reach it.
    app.run(host="0.0.0.0", debug=False, port=5000)
