"""Central configuration: paths, source flags, seeds, thresholds."""
from __future__ import annotations

import os
from dataclasses import dataclass, field
from pathlib import Path

PACKAGE_ROOT = Path(__file__).resolve().parent.parent
DATA_DIR = PACKAGE_ROOT / "data"
CACHE_DIR = DATA_DIR / "cache"
BETS_LOG = DATA_DIR / "bets.csv"

SEED = 2026

# Time-based splits (inclusive bounds). Never shuffle.
TRAIN_END = "2021-12-31"
VAL_END = "2024-12-31"
# test = VAL_END+1 day .. present


@dataclass
class SourceFlags:
    """Each data source can be toggled; loaders fall back gracefully when
    a source is disabled or unreachable (network policy, missing key)."""

    match_history: bool = True          # martj42 results (GitHub raw)
    fifa_rankings: bool = True          # Dato-Futbol mirror (GitHub raw)
    odds_history: bool = True           # football-data.co.uk archives
    odds_api: bool = bool(os.environ.get("ODDS_API_KEY"))
    club_elo: bool = False              # clubelo.com — needs squad mapping
    transfermarkt: bool = False         # ToS-sensitive; off by default
    statsbomb_xg: bool = False          # heavy; goals-based fallback used
    weather: bool = True                # open-meteo, no key
    news: bool = True                   # headlines for manual review only


@dataclass
class ValueConfig:
    min_edge: float = 0.03              # 3 percentage points
    kelly_fraction: float = 0.25        # quarter Kelly
    max_stake_frac: float = 0.02        # cap at 2% of bankroll
    market_blend_lambda: float = 0.0    # 0 = pure model; tuned on validation


@dataclass
class Config:
    sources: SourceFlags = field(default_factory=SourceFlags)
    value: ValueConfig = field(default_factory=ValueConfig)
    odds_api_key: str = os.environ.get("ODDS_API_KEY", "")
    telegram_token: str = os.environ.get("TELEGRAM_BOT_TOKEN", "")
    telegram_chat_id: str = os.environ.get("TELEGRAM_CHAT_ID", "")
    dashboard_token: str = os.environ.get("WC26_TOKEN", "")


CONFIG = Config()

URLS = {
    "results": "https://raw.githubusercontent.com/martj42/international_results/master/results.csv",
    "shootouts": "https://raw.githubusercontent.com/martj42/international_results/master/shootouts.csv",
    "fifa_ranking": "https://raw.githubusercontent.com/Dato-Futbol/fifa-ranking/master/ranking_fifa_historical.csv",
    # football-data.co.uk does not archive international closing odds; these
    # club archives are kept for the loader's unit-format compatibility and
    # to document the expected CSV schema for user-supplied odds files.
    "odds_api": "https://api.the-odds-api.com/v4/sports/soccer_fifa_world_cup/odds",
}
