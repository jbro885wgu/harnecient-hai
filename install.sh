#!/bin/sh
# hai installer stub (M0). Prints its version and does nothing else yet.
# Real install (M7): fetch the repo, build in place under $HOME/hai, write ~/.local/bin/hai.
set -e
PRODUCT="${1:-hai}"
echo "hai installer stub: product=$PRODUCT version=0.0.0-m0 (nothing installed)"
