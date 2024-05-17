#!/bin/bash

for param in "$@"
do
    #~/.platformio/packages/toolchain-xtensa-esp32@8.4.0+2021r2-patch5/bin/xtensa-esp32-elf-addr2line -pfiaC -e .pio/build/esp32-3248S035C/firmware.elf $param
    ~/.platformio/packages/toolchain-xtensa-esp32@8.4.0+2021r2-patch5/bin/xtensa-esp32-elf-addr2line -pfiaC -e .pio/build/esp32-2432S028R/firmware.elf $param
done
