#!/usr/bin/env bash
set -euo pipefail
# Install launch.sh as this game's Steam launch options first.
# Steam supplies its authentication context, Proton and the original prefix.
exec steam -applaunch 2536520 --frame-vr "$@"
