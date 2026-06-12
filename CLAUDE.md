# Claude Code — Project Memory

## Active Project: wc26-fairprice (supersedes /football_betting/)

Located in `/wc26-fairprice/`. A fair-price engine for WC2026 with hard
integrity gates — never a tip machine.

### Core rules
- **Logistic regression is the interpretable spine**; the user explicitly
  authorized a calibrated ensemble around it (LightGBM on identical
  features + Dixon-Coles bivariate Poisson for goals markets).
- **Integrity gates are non-negotiable**: the value engine stays in
  NO-BET mode unless `evaluate` proves the ensemble beats the de-vigged
  market (paired bootstrap p<0.05) on held-out data. Never soften these.
- Time-based splits only (train ≤2021, val 2022–24, test 2025→); the
  leakage unit test must keep passing.
- Real data: martj42 international results (49k matches, includes live
  WC26 calendar) + FIFA rankings, both via GitHub raw (other hosts are
  blocked by this environment's network policy).
- "No bet" is a successful output — keep the README note verbatim.

### Commands
`python -m wc26fp.cli` → refresh / train / evaluate / predict / value /
backtest / clv / wc26 today / wc26 match / dashboard / bot. Tests: `pytest tests/`.

## Legacy Project: Football Betting Assistant (prototype)

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
