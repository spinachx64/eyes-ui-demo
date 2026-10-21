#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "$0")/.." && pwd)"
# Defaults match this machine's T7 installation; export either path to override.
export IDF_TOOLS_PATH="${IDF_TOOLS_PATH:-/Volumes/T7-Shield/dev/espressif/.espressif}"
if [[ -z "${IDF_PATH:-}" ]]; then
  export IDF_PATH="$IDF_TOOLS_PATH/v6.1/esp-idf"
fi
export TMPDIR="${TMPDIR_EYES:-/Volumes/T7-Shield/dev/espressif/eyes-tmp}"
export IDF_COMPONENT_CACHE_PATH="${IDF_COMPONENT_CACHE_PATH:-/Volumes/T7-Shield/dev/espressif/component-cache}"
mkdir -p "$TMPDIR" "$IDF_COMPONENT_CACHE_PATH"
# export.sh probes optional variables that may be unset.
set +u
source "$IDF_PATH/export.sh" >/dev/null
set -u
python "$ROOT/tools/generate_presets.py"
python "$ROOT/tools/build_web.py"
if [[ $# -eq 0 ]]; then set -- build; fi
exec idf.py -C "$ROOT/examples/stopwatch" -DIDF_TARGET=esp32s3 "$@"
