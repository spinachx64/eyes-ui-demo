#!/bin/sh
set -eu
export TMPDIR=/Volumes/T7-Shield/dev/arduino/tmp
mkdir -p "$TMPDIR"
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
exec arduino-cli compile \
  --config-file /Volumes/T7-Shield/dev/arduino/arduino-cli.yaml \
  --fqbn 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,FlashMode=qio,PartitionScheme=app3M_fat9M_16MB,CDCOnBoot=cdc,USBMode=hwcdc' \
  --build-path /Volumes/T7-Shield/dev/arduino/build/eyes-demo \
  "$ROOT/firmware/eyes_demo" "$@"
