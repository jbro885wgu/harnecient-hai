#!/bin/sh
# button.sh <house_root>  - launch the hai window (W0). Same shape as the house template launcher.
# The renderer binary is built in the house tree (dev); the shipped payload carries its own copy (M7).
set -u
APP="hai"
HOUSE_ROOT="${1:-}"
[ -n "$HOUSE_ROOT" ] && [ -d "$HOUSE_ROOT" ] || { echo "$APP: need house_root as argv[1]" >&2; exit 1; }
HOUSE_ROOT="$(cd "$HOUSE_ROOT" && pwd)"
HERE="$(cd "$(dirname "$0")" && pwd)"
XHTPM="$HERE/$APP.xhtpm"
BIN="$HOUSE_ROOT/_.monads/_.livedesk-taskbar/ops/+x/khtpm_core_render.+x"
MGR="$HERE/bin/hai_manager"
[ -x "$BIN" ] || { echo "$APP: missing $BIN" >&2; exit 1; }
[ -x "$MGR" ] || { echo "$APP: missing $MGR (build it first)" >&2; exit 1; }
mkdir -p "$HERE/state"
setsid nohup "$BIN" "$HOUSE_ROOT" "$XHTPM" >/dev/null 2>&1 < /dev/null &
echo "$APP launched"
