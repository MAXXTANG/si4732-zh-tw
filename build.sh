#!/bin/bash
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
FQBN='esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc,PartitionScheme=app3M_fat9M_16MB,FlashMode=qio'
SDK_DIR="$(cd "$PROJECT_DIR/.dev/data/packages" && pwd -P)"
# SDK assertions include __FILE__ in the application. Use a public build path.
SDK_PREFIX='/build/si4732-zh-tw/arduino-cli/esp32s3/arduino-esp32-3.3.12/platform/packages'
PATH_MAPS="\"-ffile-prefix-map=$PROJECT_DIR=/build/si4732-zh-tw/source\" \"-ffile-prefix-map=$SDK_DIR=$SDK_PREFIX\""
mkdir -p "$PROJECT_DIR/.dev/build" "$PROJECT_DIR/dist"
arduino-cli compile \
  --config-file "$PROJECT_DIR/.dev/arduino-cli.yaml" \
  --fqbn "$FQBN" \
  --library "$PROJECT_DIR/vendor/TFT_eSPI" \
  --library "$PROJECT_DIR/vendor/PU2CLR_SI4735" \
  --build-property "compiler.c.extra_flags=$PATH_MAPS" \
  --build-property "compiler.cpp.extra_flags=$PATH_MAPS" \
  --build-property "compiler.S.extra_flags=$PATH_MAPS" \
  --build-path "$PROJECT_DIR/.dev/build" \
  --output-dir "$PROJECT_DIR/dist" \
  --warnings default \
  "$PROJECT_DIR/firmware/Si4732Traditional"
