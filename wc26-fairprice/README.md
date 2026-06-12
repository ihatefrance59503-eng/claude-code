# wc26-fairprice

A fair-price engine for 2026 World Cup matches with hard integrity gates.
It prices 1X2, totals, BTTS and knockout-advancement markets, compares
those prices to bookmaker odds, and **refuses to recommend stakes unless
it has proven — on held-out data — that it beats the de-vigged market.**

> "No public-data model is guaranteed to beat closing lines. This tool's
> job is to make me bet less often and at better prices, to size stakes
> responsibly, and to measure honestly (CLV, ROI confidence intervals)
> whether any edge is real. 'No bet' is a successful output."

## Quick start

```bash
cd wc26-fairprice
pip install -r requirements.txt
python -m wc26fp.cli refresh      # download/update all data sources
python -m wc26fp.cli train        # Elo -> features -> LR + GBM + ensemble
python -m wc26fp.cli evaluate     # §4 metrics table + PASS/FAIL verdict
python -m wc26fp.cli wc26 today   # price today's World Cup matches
pytest tests/                     # 32 tests: Elo, leakage, de-vig, Kelly, DC
```

## Phone setup (the part you actually asked for)

### Option A — dashboard on your phone (primary)

```bash
export WC26_TOKEN="pick-a-long-random-string"
python -m wc26fp.cli dashboard --port 8000
```

Then on your phone open `http://<your-machine-ip>:8000/?token=...`
(same Wi-Fi), or put it on the internet with a tunnel:

```bash
cloudflared tunnel --url http://localhost:8000
# open the printed trycloudflare.com URL on your phone, append ?token=...
```

**Free-host deploy (Render):** create a new Web Service from this repo,
build command `pip install -r wc26-fairprice/requirements.txt`, start
command `cd wc26-fairprice && python -m wc26fp.cli dashboard --port $PORT`,
and set the `WC26_TOKEN` env var. The free tier sleeps when idle; first
load takes ~30 s. **Do not skip the token** — without it the page is public.

### Option B — Telegram bot (secondary)

1. Talk to **@BotFather**, create a bot, copy the token.
2. `export TELEGRAM_BOT_TOKEN=...` then `python -m wc26fp.cli bot`
3. Text it: `today`, `match CAN BIH`, or `value CAN BIH 1.83 3.5 4.6`

For the daily pre-matchday push, copy `.github-workflow-daily.yml` to
`.github/workflows/daily.yml` and add `TELEGRAM_BOT_TOKEN` /
`TELEGRAM_CHAT_ID` repo secrets.

Everything the dashboard or bot shows is also a CLI command, so nothing
depends on any host being up.

## Data sources (all behind flags in `wc26fp/config.py`, all cached)

| source | what | status |
|---|---|---|
| martj42 international results (GitHub) | 49k matches 1872→today, incl. the live WC26 calendar | ✅ default on |
| self-computed Elo | K by importance, margin multiplier, home offset, neutral handling; pre-match ratings persisted per row | ✅ always |
| FIFA rankings (point totals) | merged as-of match date | ✅ default on |
| historical closing odds | **no free bulk archive exists for internationals.** Drop a CSV at `data/cache/intl_odds.csv` with columns `date,home_team,away_team,odds_home,odds_draw,odds_away` (e.g. exported from OddsPortal) | ⚠️ user-supplied |
| The Odds API (live prices) | set `ODDS_API_KEY` | ⚠️ key needed |
| clubelo / Transfermarkt squad values | squad-strength proxies | ⏸ off by default (feature slots exist, imputed 0) |
| StatsBomb/FBref xG | goals-based features used as fallback | ⏸ off by default |
| open-meteo weather, venue altitude | WC26 venues file has coords + altitude (Azteca: 2,240 m) | ✅ |
| `news <team>` | headlines for **your** review — never fed to the model | ✅ |

## Models

1. **Calibrated multinomial logistic regression** — the interpretable spine;
   `train` prints standardised coefficients (isotonic-vs-Platt chosen on validation).
2. **LightGBM** on identical features, calibrated the same way.
3. **Dixon-Coles bivariate Poisson** with time decay — correct-score grid
   → totals 1.5/2.5/3.5, BTTS, clean sheets, and ET/pens-adjusted
   "to qualify" prices in knockouts.
4. **Ensemble** — validation-log-loss-optimal blend (currently ~0.2 LR / 0.8 GBM).
   Optional market blend λ exists (`tune_market_lambda`) and is reported honestly.

Splits are strictly time-based: train ≤2021, validation 2022–24, test 2025→.
A leakage unit test rebuilds features on truncated history and asserts equality.

## The §4 verdict (current state, this machine)

```
                        n  log_loss   brier     rps
Dummy (base rates)   1315    1.0438  0.6287  0.2274
Elo-only baseline    1315    0.8356  0.4900  0.1610
Logistic regression  1315    0.8727  0.4971  0.1633
Gradient boosting    1315    0.8817  0.4891  0.1608
Ensemble             1315    0.8355  0.4889  0.1607

VERDICT: FAIL — market benchmark UNAVAILABLE (no historical odds file).
Value engine locked to NO-BET mode (override with --override-verdict).
```

Two honest observations baked into the design:

- **The ensemble barely ties the Elo-only baseline.** International
  football with public data is mostly Elo; everything else adds noise as
  often as signal. The verdict gate exists precisely because of this.
- **Without an odds benchmark the gate cannot PASS**, so `value` prints
  fair prices and edges but stakes are forced to NO BET. Supply
  `intl_odds.csv` and re-run `evaluate` to attempt the gate honestly.

## Caveats (read before staking anything)

- A ~104-match tournament is a tiny sample; variance dominates.
- International data is sparse — teams play ~10 matches/year.
- No model sees late team news. Check lineups yourself before staking;
  `news <team>` exists to help, and is deliberately not a model input.
- Group-stage draw inflation and MD3 dead-rubber damping are documented
  priors (`wc26fp/wc26.py`), not learned parameters.
- ET/pens in "to qualify" is an Elo-shrunk coin flip, not a penalties model.

## CLI reference

```
refresh | train | evaluate | predict HOME AWAY [--knockout] [--home-advantage]
value HOME AWAY OH OD OA [--bankroll N] [--override-verdict]
backtest | clv | news TEAM | wc26 today | wc26 match CAN BIH | bot | dashboard
```

Every `value` recommendation is logged to `data/bets.csv` with model and
market probabilities. Fill in `closing_odds` as kickoffs approach and run
`clv` — beating the close is the single best predictor that any of this
is actually working.
