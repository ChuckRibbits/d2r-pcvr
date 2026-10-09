#!/usr/bin/env bash
set -euo pipefail
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# Steam launch options: "/absolute/path/to/steamvr/launch.sh" %command%
# Steam Play stays unmodded. Only the explicit --frame-vr argument selects VR.
args=()
vr=0
for arg in "$@"; do
    if [[ "$arg" == "--frame-vr" ]]; then vr=1; else args+=("$arg"); fi
done
if (( vr )); then
    found=0
    for i in "${!args[@]}"; do
        if [[ "${args[$i]##*/}" == "D2R.exe" ]]; then
            game_dir="${args[$i]%/*}"
            [[ -f "$game_dir/D2RLoader.exe" ]] || { echo 'D2RLoader is not installed' >&2; exit 1; }
            args[$i]="$game_dir/D2RLoader.exe"
            # Keep Proton's game-drive mapping consistent across Steam launches.
            # The successful licensed launch used S:, whereas a missing library
            # path caused a later launch to fall back to Z: and reject its cache.
            export STEAM_COMPAT_INSTALL_PATH="$game_dir"
            library_path="${game_dir%/steamapps/common/*}/steamapps"
            [[ -d "$library_path" ]] || { echo 'Cannot determine Steam library path' >&2; exit 1; }
            export STEAM_COMPAT_LIBRARY_PATHS="$library_path${STEAM_COMPAT_LIBRARY_PATHS:+:$STEAM_COMPAT_LIBRARY_PATHS}"
            found=1
        fi
    done
    (( found )) || { echo 'Expected Steam launch command containing D2R.exe' >&2; exit 1; }
    export PROTON_LOG=1
    # Proton's default log profile traces every exception/unwind. D2R uses
    # these frequently enough to flood the log and stall gameplay. Keep errors
    # and native VKD3D warnings; Wine's global warnings also log routine file
    # opens/debug-string exceptions, so keep Wine itself at errors only.
    export WINEDEBUG="${D2R_FRAME_WINEDEBUG:--all,+timestamp,+pid,+tid,err+all}"
    export PRESSURE_VESSEL_IMPORT_OPENXR_1_RUNTIMES=1
    export PROTON_LOG_DIR="${D2R_FRAME_LOG_DIR:-$script_dir/../build-frame}"
    mkdir -p -- "$PROTON_LOG_DIR"
    {
        date --iso-8601=seconds
        printf 'Install path: %s\nLibrary paths: %s\nPrefix: %s\n' "$STEAM_COMPAT_INSTALL_PATH" "$STEAM_COMPAT_LIBRARY_PATHS" "${STEAM_COMPAT_DATA_PATH:-unset}"
        printf 'Wine debug profile: %s\n' "$WINEDEBUG"
    } >> "$PROTON_LOG_DIR/steam-launch-routing.log"
fi
if (( vr )); then
    exec python3 "$script_dir/session.py" "${args[@]}"
fi
exec "${args[@]}"
