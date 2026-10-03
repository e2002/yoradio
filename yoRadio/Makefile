# yoRadio build for Arduino Nano ESP32 via arduino-cli
#
#   make            - compile the sketch (same as `make build`)
#   make flash      - compile, then upload to the board
#   make upload     - upload the existing build without recompiling
#   make monitor    - open the serial monitor
#   make clean      - remove the build directory
#
# Override any variable on the command line, e.g.
#   make flash PORT=/dev/ttyACM1
#   make build FQBN=esp32:esp32:esp32s3

SKETCH_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
SKETCH     := $(SKETCH_DIR)yoRadio.ino

ARDUINO_CLI ?= arduino-cli
BOARD       ?= esp32:esp32:nano_nora
BOARD_OPTS  ?= PartitionScheme=spiffs,PinNumbers=byGPIONumber
FQBN        ?= $(BOARD):$(BOARD_OPTS)
BUILD_DIR   ?= $(SKETCH_DIR)build
BAUD        ?= 115200

# Auto-detect the first port reporting this board; override with PORT=...
ifeq ($(origin PORT), undefined)
PORT := $(shell $(ARDUINO_CLI) board list 2>/dev/null | awk -v b="$(BOARD)" '$$0 ~ b {print $$1; exit}')
ifeq ($(PORT),)
PORT := $(firstword $(wildcard /dev/ttyACM*))
endif
endif

COMPILE_FLAGS ?=
UPLOAD_FLAGS  ?=

.PHONY: all build flash upload monitor clean ports

all: build

build:
	$(ARDUINO_CLI) compile --fqbn "$(FQBN)" --build-path "$(BUILD_DIR)" $(COMPILE_FLAGS) "$(SKETCH_DIR)"

flash: build upload

upload: require-port
	$(ARDUINO_CLI) upload --fqbn "$(FQBN)" --port "$(PORT)" --input-dir "$(BUILD_DIR)" $(UPLOAD_FLAGS) "$(SKETCH_DIR)"

monitor: require-port
	$(ARDUINO_CLI) monitor --port "$(PORT)" --config baudrate=$(BAUD)

ports:
	$(ARDUINO_CLI) board list

clean:
	rm -rf "$(BUILD_DIR)"

require-port:
	@test -n "$(PORT)" || { echo "No board found. Plug in the Nano ESP32 or pass PORT=/dev/ttyACMx"; exit 1; }
