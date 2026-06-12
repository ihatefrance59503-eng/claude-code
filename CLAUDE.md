# Claude Code — Project Memory

## Active Project: World Cup 2026 Football Betting Assistant

Located in `/football_betting/`.

### Purpose
A Flask web app that uses **logistic regression** to generate betting predictions for World Cup 2026 matches. Covers all 48 teams across 12 groups.

### Core rule
**Always use logistic regression** as the prediction model. Do not introduce random forests, neural nets, or other models without explicit permission.

### Bet markets predicted
- Match result (1X2): home win / draw / away win
- Over/Under 2.5 goals
- Both Teams To Score (BTTS)
- Fouls over/under 25.5
- Cards over/under 3.5

### Data sources
- `data_fetcher.py` — pulls from football-data.org free API, FBref scraping, and a compiled stats table for all 48 WC2026 teams
- Stats include: goals per game, xG, shots on target, fouls per game, yellow/red cards, possession, FIFA ranking, recent form

### Architecture
- `app.py` — Flask routes: `/`, `/match`, `/teams`, `/team/<name>`, `/api/predict`
- `data_fetcher.py` — data layer (team stats, fixtures, head-to-head)
- `model.py` — logistic regression pipelines (trained on 2000 simulated international matches)
- `templates/` — Jinja2 HTML templates
- `static/style.css` — dark-mode CSS

### Running
```bash
cd football_betting
python app.py
```
Then open http://localhost:5000
