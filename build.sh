#!/bin/sh
# Builds the firmware for the Seeed XIAO 1.47" touch display (nRF52840 Plus)
# and makes a .uf2 you can drag onto the board.
#
# Needs arduino-cli with the Seeed nRF52 core (Seeeduino:nrf52, I use 1.1.13)
# and PlusPlusXiao/pp_device_material.h with your own device material.
# Anything you add after ./build.sh goes on to arduino-cli compile.
set -e
cd "$(dirname "$0")"
if [ ! -f PlusPlusXiao/pp_device_material.h ]; then
  echo "PlusPlusXiao/pp_device_material.h is missing, see the README" >&2
  exit 1
fi
FLAGS="-DPP_BOARD_DISPLAY147 -DCONFIG_NFCT_PINS_AS_GPIOS"
arduino-cli compile --fqbn Seeeduino:nrf52:xiaonRF52840Plus --build-path "$PWD/build" \
  --build-property "compiler.c.extra_flags=$FLAGS" \
  --build-property "compiler.cpp.extra_flags=$FLAGS" \
  "$@" PlusPlusXiao
DATA=$(arduino-cli config get directories.data)
UF2CONV=$(ls "$DATA"/packages/Seeeduino/hardware/nrf52/*/tools/uf2conv/uf2conv.py | tail -1)
python3 "$UF2CONV" -f 0xADA52840 -c -o build/PlusPlusXiao.uf2 build/PlusPlusXiao.ino.hex
echo "Done: build/PlusPlusXiao.uf2"
