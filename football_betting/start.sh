#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# WC2026 Betting Assistant — launcher
# Usage:
#   ./start.sh          → local only (http://localhost:5000)
#   ./start.sh --phone  → creates a public HTTPS URL for your phone
# ─────────────────────────────────────────────────────────────────────────────
set -e
cd "$(dirname "$0")"

MODE="${1:-local}"

# Start Flask in background
echo "Starting Flask app..."
python app.py &
FLASK_PID=$!
sleep 2

if [ "$MODE" = "--phone" ]; then
  if ! command -v cloudflared &>/dev/null; then
    echo "Installing cloudflared tunnel..."
    curl -fsSL https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64 \
      -o /usr/local/bin/cloudflared
    chmod +x /usr/local/bin/cloudflared
  fi
  echo ""
  echo "Creating public tunnel for phone access..."
  echo "Your phone URL will appear below (look for the trycloudflare.com link):"
  echo "─────────────────────────────────────────────────────────────────────"
  cloudflared tunnel --url http://localhost:5000 &
  TUNNEL_PID=$!
  echo ""
  echo "Open that URL on your phone. Press Ctrl+C to stop everything."
  trap "kill $FLASK_PID $TUNNEL_PID 2>/dev/null" EXIT
  wait
else
  echo ""
  echo "App running at: http://localhost:5000"
  echo "To share with your phone, run:  ./start.sh --phone"
  echo "Press Ctrl+C to stop."
  trap "kill $FLASK_PID 2>/dev/null" EXIT
  wait
fi
