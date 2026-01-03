#!/bin/bash
set -e

echo "--- Starting building yoRadio ---"

# 1. Check existance of myoptions.h w volume
if [ ! -f "/build/myoptions.h" ]; then
    echo "ERROR: myoptions.h not fund in mounted volume!"
    echo "Make sure that myoptions.h is in the folder mounted to /build."
    exit 1
fi

# 2. Copying myoptions.h to source dir
echo "Copying myoptions.h..."
cp /build/myoptions.h /app/yoradio/yoRadio/myoptions.h

# 3. Configuring board and partition shema same as in Quick start
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"

echo "Compilation for: $FQBN..."
echo "This may take a few minutes..."

# Add --clean flag, to make sure that everyting will rebuild with a fresh memory map
arduino-cli compile --fqbn $FQBN --output-dir /build/bin --clean /app/yoradio/yoRadio/yoRadio.ino

echo "---"
echo "SUCCESS! Compiled files (.bin included) should be in your /bin folder."