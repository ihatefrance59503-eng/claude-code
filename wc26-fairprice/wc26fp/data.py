"""Data layer: download, cache, and load every source behind a flag.

All loaders return pandas DataFrames and never raise on network failure —
they fall back to the cached copy, or an empty frame with the right schema.
"""
from __future__ import annotations

import sys
from pathlib import Path

import numpy as np
import pandas as pd
import requests

from .config import CACHE_DIR, CONFIG, URLS

CACHE_DIR.mkdir(parents=True, exist_ok=True)

# Competition importance tiers (drives Elo K and form weighting)
TIER = {
    "FIFA World Cup": 4.0,
    "FIFA World Cup qualification": 2.5,
    "UEFA Euro": 3.0,
    "Copa América": 3.0,
    "African Cup of Nations": 3.0,
    "AFC Asian Cup": 3.0,
    "CONCACAF Championship": 3.0,
    "Gold Cup": 3.0,
    "UEFA Nations League": 2.0,
    "CONCACAF Nations League": 2.0,
    "Confederations Cup": 2.5,
    "Friendly": 1.0,
}
DEFAULT_TIER = 2.0  # other qualifiers / minor tournaments

# Known high-altitude host cities (metres). Used as a historical feature;
# WC26 venues carry exact altitude in data/wc26_venues.csv.
ALTITUDE = {
    "La Paz": 3640, "El Alto": 4150, "Quito": 2850, "Bogotá": 2640,
    "Bogota": 2640, "Cusco": 3399, "Sucre": 2810, "Cochabamba": 2558,
    "Mexico City": 2240, "Toluca": 2660, "Puebla": 2135, "Guadalajara": 1566,
    "Zapopan": 1566, "Addis Ababa": 2355, "Asmara": 2325, "Sana'a": 2250,
    "Thimphu": 2334, "Kathmandu": 1400, "Nairobi": 1795, "Johannesburg": 1753,
    "Harare": 1483, "Kampala": 1190, "Arequipa": 2335, "Medellín": 1495,
}


def _download(url: str, dest: Path, timeout: int = 60) -> bool:
    try:
        resp = requests.get(url, timeout=timeout)
        resp.raise_for_status()
        dest.write_bytes(resp.content)
        return True
    except Exception as exc:  # noqa: BLE001 — any failure means "use cache"
        print(f"  [warn] could not fetch {url}: {exc}", file=sys.stderr)
        return False


def refresh(force: bool = False) -> dict[str, bool]:
    """Idempotently update all enabled sources. Returns per-source status."""
    status: dict[str, bool] = {}
    jobs = []
    if CONFIG.sources.match_history:
        jobs += [("results", URLS["results"]), ("shootouts", URLS["shootouts"])]
    if CONFIG.sources.fifa_rankings:
        jobs.append(("fifa_ranking", URLS["fifa_ranking"]))
    for name, url in jobs:
        dest = CACHE_DIR / f"{name}.csv"
        if dest.exists() and not force:
            # still re-download: these files append new rows over time
            status[name] = _download(url, dest) or True  # cache survives failure
        else:
            status[name] = _download(url, dest)
    return status


def load_results() -> pd.DataFrame:
    """Full match history. Played matches only (scores present)."""
    df = _load_results_raw()
    df = df.dropna(subset=["home_score", "away_score"]).copy()
    df["home_score"] = df["home_score"].astype(int)
    df["away_score"] = df["away_score"].astype(int)
    return df.reset_index(drop=True)


def load_future_fixtures() -> pd.DataFrame:
    """Scheduled matches (scores NA) — includes the WC2026 calendar."""
    df = _load_results_raw()
    return df[df["home_score"].isna()].reset_index(drop=True)


def _load_results_raw() -> pd.DataFrame:
    path = CACHE_DIR / "results.csv"
    if not path.exists():
        refresh()
    df = pd.read_csv(path, na_values=["NA"])
    df["date"] = pd.to_datetime(df["date"])
    df["home_score"] = pd.to_numeric(df["home_score"], errors="coerce")
    df["away_score"] = pd.to_numeric(df["away_score"], errors="coerce")
    df["neutral"] = df["neutral"].astype(bool)
    df["tier"] = df["tournament"].map(TIER).fillna(DEFAULT_TIER)
    df["altitude"] = df["city"].map(ALTITUDE).fillna(0.0)
    # Host-nation: home side playing in its own country and not flagged neutral
    df["host"] = (~df["neutral"]) & (df["home_team"] == df["country"])
    df = df.sort_values("date", kind="stable").reset_index(drop=True)
    return df


def load_fifa_rankings() -> pd.DataFrame:
    """FIFA point totals (long format: team, total_points, date)."""
    path = CACHE_DIR / "fifa_ranking.csv"
    if not path.exists() and CONFIG.sources.fifa_rankings:
        refresh()
    if not path.exists():
        return pd.DataFrame(columns=["team", "total_points", "date"])
    df = pd.read_csv(path, na_values=["NA"])
    df["date"] = pd.to_datetime(df["date"])
    df["team"] = df["team"].str.replace(r" \(unranked\)", "", regex=True)
    df["total_points"] = pd.to_numeric(df["total_points"], errors="coerce")
    df = df.dropna(subset=["total_points"])
    return df[["team", "total_points", "date"]].sort_values("date")


# ---------------------------------------------------------------------------
# Odds sources
# ---------------------------------------------------------------------------

ODDS_SCHEMA = ["date", "home_team", "away_team", "odds_home", "odds_draw", "odds_away"]


def load_historical_odds() -> pd.DataFrame:
    """Historical closing 1X2 odds for internationals.

    football-data.co.uk archives club football only; there is no free bulk
    archive of international closing odds. Drop a CSV with the schema
    {date, home_team, away_team, odds_home, odds_draw, odds_away}
    into data/cache/intl_odds.csv (e.g. exported from OddsPortal) and it
    will be picked up here. Without it, the market benchmark — and therefore
    the §4 PASS gate — is unavailable, and the value engine stays in
    no-bet mode.
    """
    path = CACHE_DIR / "intl_odds.csv"
    if not path.exists():
        return pd.DataFrame(columns=ODDS_SCHEMA)
    df = pd.read_csv(path)
    df["date"] = pd.to_datetime(df["date"])
    missing = set(ODDS_SCHEMA) - set(df.columns)
    if missing:
        raise ValueError(f"intl_odds.csv missing columns: {missing}")
    return df


def fetch_live_odds() -> pd.DataFrame:
    """Current WC odds from The Odds API (needs ODDS_API_KEY)."""
    if not CONFIG.sources.odds_api:
        return pd.DataFrame(columns=ODDS_SCHEMA)
    try:
        resp = requests.get(
            URLS["odds_api"],
            params={"apiKey": CONFIG.odds_api_key, "regions": "eu", "markets": "h2h"},
            timeout=20,
        )
        resp.raise_for_status()
        rows = []
        for game in resp.json():
            best: dict[str, float] = {}
            for bm in game.get("bookmakers", []):
                for mkt in bm.get("markets", []):
                    if mkt["key"] != "h2h":
                        continue
                    for out in mkt["outcomes"]:
                        best[out["name"]] = max(best.get(out["name"], 0.0), out["price"])
            if len(best) >= 3:
                rows.append({
                    "date": pd.to_datetime(game["commence_time"]).tz_localize(None),
                    "home_team": game["home_team"],
                    "away_team": game["away_team"],
                    "odds_home": best.get(game["home_team"], np.nan),
                    "odds_draw": best.get("Draw", np.nan),
                    "odds_away": best.get(game["away_team"], np.nan),
                })
        return pd.DataFrame(rows, columns=ODDS_SCHEMA)
    except Exception as exc:  # noqa: BLE001
        print(f"  [warn] Odds API unavailable: {exc}", file=sys.stderr)
        return pd.DataFrame(columns=ODDS_SCHEMA)


# ---------------------------------------------------------------------------
# Weather (open-meteo, free, no key) — graceful when blocked
# ---------------------------------------------------------------------------

def fetch_weather(lat: float, lon: float, date_iso: str, hour: int = 18) -> dict:
    if not CONFIG.sources.weather:
        return {}
    try:
        resp = requests.get(
            "https://api.open-meteo.com/v1/forecast",
            params={
                "latitude": lat, "longitude": lon,
                "hourly": "temperature_2m,relative_humidity_2m",
                "start_date": date_iso, "end_date": date_iso,
            },
            timeout=10,
        )
        resp.raise_for_status()
        h = resp.json()["hourly"]
        return {
            "temp_c": h["temperature_2m"][hour],
            "humidity": h["relative_humidity_2m"][hour],
        }
    except Exception:  # noqa: BLE001
        return {}


# ---------------------------------------------------------------------------
# News (manual review only — never fed into the model)
# ---------------------------------------------------------------------------

def fetch_news(team: str, limit: int = 5) -> list[str]:
    """Recent headlines for one team. LOWER-TRUST: for human review only."""
    if not CONFIG.sources.news:
        return []
    try:
        resp = requests.get(
            "https://news.google.com/rss/search",
            params={"q": f"{team} national football team", "hl": "en"},
            timeout=10,
        )
        resp.raise_for_status()
        import xml.etree.ElementTree as ET

        root = ET.fromstring(resp.content)
        return [item.findtext("title", "") for item in root.iter("item")][:limit]
    except Exception:  # noqa: BLE001
        return []
