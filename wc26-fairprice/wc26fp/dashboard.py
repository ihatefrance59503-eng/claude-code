"""Mobile-first dashboard: today's WC matches, model vs market, edge badges.

Token-protected: set WC26_TOKEN and open  /?token=YOURTOKEN  on your phone.
Everything shown here is also available from the CLI — the host being down
never blocks you.
"""
from __future__ import annotations

import html

from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import HTMLResponse, JSONResponse

from .config import CONFIG
from .evaluate import VERDICT_PATH, verdict_passed

CSS = """
:root{--bg:#0d1117;--card:#161b22;--border:#30363d;--text:#e6edf3;
--muted:#8b949e;--green:#3fb950;--red:#f85149;--gold:#d29922}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,system-ui,sans-serif;background:var(--bg);
color:var(--text);padding:12px;max-width:560px;margin:0 auto}
h1{font-size:1.15rem;color:var(--gold);margin:8px 0 2px}
.sub{color:var(--muted);font-size:.75rem;margin-bottom:10px}
.verdict{padding:10px 12px;border-radius:10px;font-size:.8rem;margin-bottom:12px;
border:1px solid var(--border)}
.verdict.fail{border-left:4px solid var(--red)}
.verdict.pass{border-left:4px solid var(--green)}
.card{background:var(--card);border:1px solid var(--border);border-radius:12px;
padding:14px;margin-bottom:12px}
.teams{font-size:1rem;font-weight:700;display:flex;justify-content:space-between;
align-items:center;gap:8px}
.venue{color:var(--muted);font-size:.72rem;margin:2px 0 10px}
.bar{display:flex;height:22px;border-radius:6px;overflow:hidden;font-size:.68rem;
font-weight:700;color:#000;margin-bottom:8px}
.bar div{display:flex;align-items:center;justify-content:center;min-width:34px}
.h{background:var(--green)}.d{background:var(--gold)}.a{background:#58a6ff}
.row{display:flex;justify-content:space-between;font-size:.8rem;padding:3px 0;
color:var(--muted)}
.row b{color:var(--text)}
.badge{padding:2px 8px;border-radius:6px;font-size:.7rem;font-weight:700}
.badge.nobet{background:#21262d;color:var(--muted);border:1px solid var(--border)}
.badge.bet{background:var(--green);color:#000}
.footer{color:var(--muted);font-size:.68rem;text-align:center;margin-top:16px;
line-height:1.5}
"""


def _check_token(request: Request) -> None:
    expected = CONFIG.dashboard_token
    if not expected:
        return  # token unset => open (warn in README)
    supplied = request.query_params.get("token", "") or \
        request.headers.get("x-wc26-token", "")
    if supplied != expected:
        raise HTTPException(status_code=401, detail="bad or missing token")


def create_app() -> FastAPI:
    app = FastAPI(title="wc26-fairprice")

    # Heavy imports/model-load deferred so the app boots fast and /health
    # works even before training.
    state: dict = {"pricer": None}

    def pricer():
        if state["pricer"] is None:
            from .value import Pricer
            state["pricer"] = Pricer()
        return state["pricer"]

    @app.get("/health")
    def health() -> dict:
        return {"ok": True, "verdict": "PASS" if verdict_passed() else "FAIL"}

    @app.get("/api/today")
    def api_today(request: Request):
        _check_token(request)
        from .wc26 import todays_matches
        matches = todays_matches()
        out = []
        for _, m in matches.iterrows():
            host = m["home_team"] == m["country"]
            p = pricer().price(m["home_team"], m["away_team"],
                               when=str(m["date"].date()),
                               neutral=not host, stage=m["stage"])
            p["venue"] = f"{m['city']} ({m['altitude_m']:.0f}m)"
            out.append(p)
        return JSONResponse(out)

    @app.get("/", response_class=HTMLResponse)
    def index(request: Request):
        _check_token(request)
        from .wc26 import todays_matches
        verdict_text = (VERDICT_PATH.read_text().splitlines()[1]
                        if VERDICT_PATH.exists() else "not evaluated yet")
        passed = verdict_passed()
        vclass, vlabel = ("pass", "PASS") if passed else ("fail", "FAIL")

        matches = todays_matches()
        cards = []
        for _, m in matches.iterrows():
            host = m["home_team"] == m["country"]
            p = pricer().price(m["home_team"], m["away_team"],
                               when=str(m["date"].date()),
                               neutral=not host, stage=m["stage"])
            ph, pd_, pa = p["p_home"], p["p_draw"], p["p_away"]
            gm = p["goals_model"]
            cards.append(f"""
<div class="card">
  <div class="teams"><span>{html.escape(p['home'])}</span>
    <span style="color:var(--muted);font-size:.75rem">v</span>
    <span>{html.escape(p['away'])}</span></div>
  <div class="venue">{html.escape(str(m['city']))} &middot;
    alt {m['altitude_m']:.0f}m &middot; {m['date'].date()}
    {'&middot; HOST' if host else ''}</div>
  <div class="bar">
    <div class="h" style="flex:{ph:.3f}">{ph*100:.0f}%</div>
    <div class="d" style="flex:{pd_:.3f}">{pd_*100:.0f}%</div>
    <div class="a" style="flex:{pa:.3f}">{pa*100:.0f}%</div>
  </div>
  <div class="row"><span>Fair odds (1X2)</span>
    <b>{p['fair_home']} / {p['fair_draw']} / {p['fair_away']}</b></div>
  <div class="row"><span>Over 2.5 goals</span>
    <b>{gm['over_2.5']*100:.0f}% (fair {p['fair_over25']})</b></div>
  <div class="row"><span>BTTS yes</span>
    <b>{gm['btts_yes']*100:.0f}% (fair {p['fair_btts_yes']})</b></div>
  <div class="row"><span>Stake suggestion</span>
    <span class="badge {'bet' if passed else 'nobet'}">
      {'enter odds via CLI `value`' if passed else 'NO BET (gate: FAIL)'}</span></div>
</div>""")

        body = "".join(cards) or '<div class="card">No WC26 matches today.</div>'
        return f"""<!doctype html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>WC26 fair prices</title><style>{CSS}</style></head><body>
<h1>WC26 Fair-Price Engine</h1>
<div class="sub">model vs market &middot; logistic-regression spine &middot; CLI-equivalent</div>
<div class="verdict {vclass}"><b>Verdict: {vlabel}</b><br>{html.escape(verdict_text)}</div>
{body}
<div class="footer">Fair prices, not tips. &ldquo;No bet&rdquo; is a successful output.<br>
For entertainment/analysis. Bet responsibly.</div>
</body></html>"""

    return app
