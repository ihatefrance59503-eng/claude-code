"""Telegram bot via plain long-polling (no heavy dependency).

Set TELEGRAM_BOT_TOKEN (from @BotFather), then `python -m wc26fp.cli bot`.
Commands you can text it:
    today                     — price today's WC26 matches
    match CAN BIH             — fair prices for one fixture
    value CAN BIH 1.83 3.5 4.6 — edge + stake verdict vs offered 1X2 odds
"""
from __future__ import annotations

import time

import requests

from .config import CONFIG

API = "https://api.telegram.org/bot{token}/{method}"


def _call(method: str, **params):
    resp = requests.post(API.format(token=CONFIG.telegram_token, method=method),
                         json=params, timeout=35)
    resp.raise_for_status()
    return resp.json()


def send_message(chat_id: str | int, text: str) -> None:
    _call("sendMessage", chat_id=chat_id, text=text)


def _fmt_price(p: dict) -> str:
    lines = [
        f"{p['home']} v {p['away']}  ({p.get('stage', 'group')})",
        f"1X2  {p['p_home']:.3f} / {p['p_draw']:.3f} / {p['p_away']:.3f}",
        f"fair {p['fair_home']} / {p['fair_draw']} / {p['fair_away']}",
        f"O2.5 {p['goals_model']['over_2.5']:.3f} (fair {p['fair_over25']})  "
        f"BTTS {p['goals_model']['btts_yes']:.3f} (fair {p['fair_btts_yes']})",
    ]
    if "to_qualify" in p:
        lines.append(f"to qualify: {p['to_qualify']['home_qualify']:.3f} / "
                     f"{p['to_qualify']['away_qualify']:.3f}")
    return "\n".join(lines)


def handle(text: str) -> str:
    from .value import Pricer
    parts = text.strip().split()
    if not parts:
        return "commands: today | match A B | value A B oh od oa"
    cmd = parts[0].lower().lstrip("/")
    try:
        if cmd == "today":
            from .wc26 import todays_matches
            matches = todays_matches()
            if matches.empty:
                return "No WC26 matches today."
            pricer = Pricer()
            outs = []
            for _, m in matches.iterrows():
                host = m["home_team"] == m["country"]
                p = pricer.price(m["home_team"], m["away_team"],
                                 when=str(m["date"].date()),
                                 neutral=not host, stage=m["stage"])
                outs.append(_fmt_price(p))
            return "\n\n".join(outs)
        if cmd == "match" and len(parts) >= 3:
            return _fmt_price(Pricer().price(parts[1], parts[2]))
        if cmd == "value" and len(parts) >= 6:
            odds = [float(x) for x in parts[3:6]]
            res = Pricer().value(parts[1], parts[2], odds)
            lines = [f"{res['home']} v {res['away']}  "
                     f"(gate: {'PASS' if res['gate_passed'] else 'FAIL'})"]
            for r in res["assessment"]:
                lines.append(f"{r['selection']}: model {r['model_prob']:.3f} "
                             f"@ {r['offered_odds']}  edge {r['edge_pp']:+.1f}pp  "
                             f"-> {r['decision']}")
            return "\n".join(lines)
        return "commands: today | match A B | value A B oh od oa"
    except Exception as exc:  # noqa: BLE001 — bot must not crash on bad input
        return f"error: {exc}"


def run_bot() -> None:
    if not CONFIG.telegram_token:
        raise SystemExit("set TELEGRAM_BOT_TOKEN first (talk to @BotFather)")
    print("bot polling — text it `today` or `value CAN BIH 1.83 3.5 4.6`")
    offset = 0
    while True:
        try:
            updates = _call("getUpdates", offset=offset, timeout=30)
            for u in updates.get("result", []):
                offset = u["update_id"] + 1
                msg = u.get("message") or {}
                if "text" in msg:
                    send_message(msg["chat"]["id"], handle(msg["text"]))
        except KeyboardInterrupt:
            break
        except Exception as exc:  # noqa: BLE001
            print(f"poll error (retrying): {exc}")
            time.sleep(5)


def push_daily_flags() -> None:
    """Used by the GitHub Actions cron: price today's matches and push any
    PASS-gated value flags to TELEGRAM_CHAT_ID before the matchday."""
    if not (CONFIG.telegram_token and CONFIG.telegram_chat_id):
        print("telegram not configured; printing instead\n" + handle("today"))
        return
    send_message(CONFIG.telegram_chat_id, handle("today"))
