#!/bin/sh
# hai installer. Builds hai from source into your home folder. No sudo, nothing outside $HOME, no language model, no network use after install.
#
#   curl -fsSL https://raw.githubusercontent.com/jbro885wgu/harnecient-hai/hai/install.sh | sh -s -- hai
#
# What it does: downloads the source (or copies it from HAI_SRC), compiles two small C programs with cc, gcc or clang, and writes a "hai" command.
#   program and data:  $HAI_HOME      (default ~/.local/share/hai)
#   your chat ledger:  $HAI_DATA      (default ~/.local/share/hai-chat, file chat.ledger: yours, never overwritten by an update)
#   the command:       ~/.local/bin/hai
# Running it again updates the program and keeps your ledger. Remove the program with:  sh install.sh uninstall
# Test without the network:  HAI_SRC=/path/to/a/checkout HOME=/tmp/somewhere sh install.sh hai
set -eu

PRODUCT="${1:-hai}"
REPO="${HAI_REPO:-jbro885wgu/harnecient-hai}"
REF="${HAI_REF:-hai}"
HAI_HOME="${HAI_HOME:-$HOME/.local/share/hai}"
HAI_DATA="${HAI_DATA:-$HOME/.local/share/hai-chat}"
BIN_DIR="${HAI_BIN:-$HOME/.local/bin}"

say() { printf 'hai install: %s\n' "$*"; }
die() { printf 'hai install: %s\n' "$*" >&2; exit 1; }

if [ "$PRODUCT" = "uninstall" ]; then
  rm -rf "$HAI_HOME" "$BIN_DIR/hai"
  say "removed $HAI_HOME and $BIN_DIR/hai. Your chat ledger is kept in $HAI_DATA (delete that folder yourself if you want it gone)."
  exit 0
fi
[ "$PRODUCT" = "hai" ] || die "unknown product '$PRODUCT' (only 'hai' exists)"

CC=""
for c in cc gcc clang; do command -v "$c" >/dev/null 2>&1 && { CC="$c"; break; }; done
[ -n "$CC" ] || die "no C compiler found (cc, gcc or clang). Install one (for example: sudo apt install build-essential) and run this again."

WORK="$(mktemp -d "${TMPDIR:-/tmp}/hai-install.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT INT TERM

if [ -n "${HAI_SRC:-}" ]; then
  [ -f "$HAI_SRC/ops/hai.c" ] || die "HAI_SRC=$HAI_SRC has no ops/hai.c"
  say "using the local source $HAI_SRC"
  mkdir -p "$WORK/src"
  cp -R "$HAI_SRC/ops" "$HAI_SRC/chat" "$WORK/src/"
else
  command -v curl >/dev/null 2>&1 || die "curl is needed to download hai"
  command -v tar >/dev/null 2>&1 || die "tar is needed to unpack hai"
  say "downloading github.com/$REPO ($REF)"
  curl -fsSL "https://github.com/$REPO/archive/refs/heads/$REF.tar.gz" -o "$WORK/hai.tar.gz" || die "download failed (is the repository public and the branch '$REF' pushed?)"
  mkdir -p "$WORK/unpack" && tar -xzf "$WORK/hai.tar.gz" -C "$WORK/unpack" || die "could not unpack the download"
  SRC="$(find "$WORK/unpack" -maxdepth 1 -mindepth 1 -type d | head -n 1)"
  [ -f "$SRC/ops/hai.c" ] || die "the download has no ops/hai.c (wrong branch '$REF'?)"
  mv "$SRC" "$WORK/src"
fi

say "compiling with $CC"
mkdir -p "$WORK/build"
for tool in hai hai_chat; do
  "$CC" -std=gnu11 -O2 -I "$WORK/src/ops" -o "$WORK/build/$tool" "$WORK/src/ops/$tool.c" "$WORK/src/ops/hai_chat_core.c" || die "compile of $tool failed"
done

# replace the installed program only after everything compiled; the ledger folder is never touched
mkdir -p "$HAI_HOME" "$HAI_DATA" "$BIN_DIR"
rm -rf "$HAI_HOME/chat" "$HAI_HOME/bin"
mkdir -p "$HAI_HOME/bin"
cp -R "$WORK/src/chat" "$HAI_HOME/chat"
cp "$WORK/build/hai" "$WORK/build/hai_chat" "$HAI_HOME/bin/"

cat > "$BIN_DIR/hai" <<EOF
#!/bin/sh
# written by install.sh: runs hai on the shared data with YOUR ledger. "hai bank" and "hai why" are read-only dumps of that ledger.
HOME_DIR="$HAI_HOME"
LEDGER="\${HAI_LEDGER:-$HAI_DATA/chat.ledger}"
case "\${1:-}" in
  bank) exec "\$HOME_DIR/bin/hai_chat" bank "\$HOME_DIR/chat/data" "\$LEDGER" ;;
  why) exec "\$HOME_DIR/bin/hai_chat" why "\$LEDGER" "\${2:-last}" ;;
  where) echo "program: \$HOME_DIR"; echo "your ledger: \$LEDGER"; exit 0 ;;
esac
exec "\$HOME_DIR/bin/hai" "\$HOME_DIR/chat/data" "\$LEDGER"
EOF
chmod 755 "$BIN_DIR/hai"

say "installed. Type:  hai"
case ":$PATH:" in
  *":$BIN_DIR:"*) ;;
  *) say "$BIN_DIR is not on your PATH yet. Add this line to your shell profile:   export PATH=\"$BIN_DIR:\$PATH\"" ;;
esac
say "try:   hai        then   /pack lyfedesk   or   /teach house = the desktop we work in"
say "see what it knows:  hai bank      why it answered:  hai why      where things are:  hai where"
