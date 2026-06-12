"""CLI: refresh, train, evaluate, predict, value, backtest, clv, wc26, news, bot."""
from __future__ import annotations

import json

import typer

app = typer.Typer(help="wc26-fairprice: World Cup 2026 fair-price engine",
                  no_args_is_help=True, pretty_exceptions_enable=False)
wc26_app = typer.Typer(help="World Cup 2026 tournament mode", no_args_is_help=True)
app.add_typer(wc26_app, name="wc26")


@app.command()
def refresh(force: bool = typer.Option(False, help="re-download even if cached")):
    """Idempotently update all enabled data sources."""
    from .data import refresh as _refresh
    status = _refresh(force=force)
    for name, ok in status.items():
        typer.echo(f"  {name}: {'updated' if ok else 'FAILED (using cache)'}")


@app.command()
def train():
    """Train LR + GBM + ensemble and the Dixon-Coles goals model."""
    from .data import load_results
    from .features import build_features
    from .models import save_ensemble, train_models

    typer.echo("Building features (Elo, FIFA, form, venue)...")
    feats = build_features(load_results())
    typer.echo("Training models (time-based splits, no shuffling)...")
    ens = train_models(feats)
    save_ensemble(ens)
    typer.echo("\nLogistic regression coefficients (standardised, top |home_win|):")
    coefs = ens.lr_coefficients.reindex(
        ens.lr_coefficients["home_win"].abs().sort_values(ascending=False).index)
    typer.echo(coefs.head(10).round(3).to_string())
    typer.echo("\nSaved to data/models.pkl")
    typer.echo("Fitting Dixon-Coles goals model (~2 min)...")
    from .value import fit_and_save_dc
    fit_and_save_dc()
    typer.echo("Saved to data/dc.pkl")


@app.command()
def evaluate():
    """Held-out test metrics vs baselines and the market; prints the verdict."""
    from .data import load_historical_odds, load_results
    from .evaluate import evaluate as _evaluate
    from .features import build_features
    from .models import load_ensemble

    feats = build_features(load_results())
    odds = load_historical_odds()
    verdict = _evaluate(load_ensemble(), feats, odds)
    typer.echo(verdict.table.to_string())
    typer.echo("\n" + "=" * 72)
    typer.echo(verdict.banner)
    typer.echo("=" * 72)


@app.command()
def predict(home: str, away: str,
            date: str = typer.Option(None, help="match date YYYY-MM-DD"),
            knockout: bool = typer.Option(False, help="knockout stage pricing"),
            home_advantage: bool = typer.Option(False, "--home-advantage",
                                                help="not a neutral venue")):
    """Fair prices for 1X2, totals, BTTS (and 'to qualify' in knockouts)."""
    from .value import Pricer
    p = Pricer().price(home, away, when=date,
                       neutral=not home_advantage,
                       stage="knockout" if knockout else "group")
    typer.echo(json.dumps(p, indent=2, default=str))


@app.command()
def value(home: str, away: str,
          odds_home: float, odds_draw: float, odds_away: float,
          bankroll: float = typer.Option(1000.0),
          override_verdict: bool = typer.Option(
              False, help="recommend stakes even when the verdict gate FAILED")):
    """Edge + quarter-Kelly stakes vs offered 1X2 odds. Logs recommendations."""
    from .value import Pricer, log_bet
    res = Pricer().value(home, away, [odds_home, odds_draw, odds_away],
                         bankroll=bankroll, override=override_verdict)
    typer.echo(json.dumps(res, indent=2, default=str))
    for row in res["assessment"]:
        if row["stake"] > 0:
            log_bet("", res["home"], res["away"], "1X2", row["selection"],
                    row["model_prob"], row["offered_odds"], row["market_prob"],
                    row["stake"], bankroll)
            typer.echo(f"logged to data/bets.csv: {row['selection']} "
                       f"stake {row['stake']}")


@app.command()
def backtest(bankroll: float = typer.Option(1000.0)):
    """Walk-forward backtest on historical odds (needs intl_odds.csv)."""
    from .backtest import run_backtest
    from .data import load_historical_odds, load_results
    from .features import build_features
    from .models import load_ensemble

    odds = load_historical_odds()
    if odds.empty:
        typer.echo("No historical odds available (data/cache/intl_odds.csv). "
                   "See README §data for the expected schema.")
        raise typer.Exit(1)
    feats = build_features(load_results())
    typer.echo(json.dumps(run_backtest(load_ensemble(), feats, odds, bankroll),
                          indent=2, default=str))


@app.command()
def clv():
    """Score logged bets against closing lines."""
    from .value import clv_report
    rep = clv_report()
    typer.echo(rep if isinstance(rep, str) else rep.to_string(index=False))


@app.command()
def news(team: str, limit: int = 5):
    """Recent headlines for a team. LOWER-TRUST: for your review only —
    never fed into the model."""
    from .data import fetch_news
    from .wc26 import resolve_team
    headlines = fetch_news(resolve_team(team), limit)
    if not headlines:
        typer.echo("(no headlines reachable — source may be blocked here)")
    for h in headlines:
        typer.echo(f"  - {h}")


@wc26_app.command("today")
def wc26_today(date: str = typer.Option(None, help="override 'today' YYYY-MM-DD")):
    """Price all of today's World Cup matches."""
    from .value import Pricer
    from .wc26 import todays_matches
    matches = todays_matches(date)
    if matches.empty:
        typer.echo("No WC26 matches scheduled today.")
        raise typer.Exit()
    pricer = Pricer()
    for _, m in matches.iterrows():
        host = m["home_team"] in {"United States", "Canada", "Mexico"} \
            and m["country"] == m["home_team"]
        p = pricer.price(m["home_team"], m["away_team"],
                         when=str(m["date"].date()), neutral=not host,
                         stage=m["stage"])
        typer.echo(f"\n{m['date'].date()}  {m['home_team']} v {m['away_team']}"
                   f"  ({m['city']}, alt {m['altitude_m']:.0f}m)")
        typer.echo(f"  1X2: {p['p_home']:.3f} / {p['p_draw']:.3f} / {p['p_away']:.3f}"
                   f"   fair odds {p['fair_home']} / {p['fair_draw']} / {p['fair_away']}")
        gm = p["goals_model"]
        typer.echo(f"  O2.5 {gm['over_2.5']:.3f} (fair {p['fair_over25']})  "
                   f"BTTS {gm['btts_yes']:.3f} (fair {p['fair_btts_yes']})")


@wc26_app.command("match")
def wc26_match(team_a: str, team_b: str):
    """Price one WC26 fixture by team names or codes, e.g. `wc26 match CAN BIH`."""
    from .value import Pricer
    from .wc26 import find_match, resolve_team
    a, b = resolve_team(team_a), resolve_team(team_b)
    fx = find_match(a, b)
    if fx is None:
        typer.echo(f"No scheduled WC26 fixture found for {a} v {b}; "
                   "pricing as a neutral-venue group match instead.")
        p = Pricer().price(a, b)
    else:
        host = fx["home_team"] == fx["country"]
        p = Pricer().price(fx["home_team"], fx["away_team"],
                           when=str(fx["date"].date()), neutral=not host,
                           stage=fx["stage"])
        p["venue"] = f"{fx['city']} (alt {fx['altitude_m']:.0f}m)"
        p["date"] = str(fx["date"].date())
    typer.echo(json.dumps(p, indent=2, default=str))


@app.command()
def bot():
    """Telegram bot: text `value CAN BIH 1.83 3.5 4.6` and get the verdict back."""
    from .bot import run_bot
    run_bot()


@app.command()
def dashboard(port: int = 8000, host: str = "0.0.0.0"):
    """Mobile-first web dashboard (token-protected via WC26_TOKEN)."""
    import uvicorn
    from .dashboard import create_app
    uvicorn.run(create_app(), host=host, port=port)


if __name__ == "__main__":
    app()
