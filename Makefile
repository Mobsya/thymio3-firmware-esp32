ADF_REPO ?= https://github.com/Mobsya/esp-adf.git
ADF_REF ?= release/v2.4
ADF_PATH ?= $(CURDIR)/.deps/esp-adf_release2.4
IDF_TARGET ?= esp32
BUILD_DIR ?= build
DIST_DIR ?= dist
FIRMWARE_PROJECT_NAME ?= thymio3-esp32-firmware

export ADF_REPO
export ADF_REF
export ADF_PATH
export IDF_TARGET
export BUILD_DIR
export DIST_DIR
export FIRMWARE_PROJECT_NAME

.DEFAULT_GOAL := help

.PHONY: help setup firmware package release clean

help:
	@printf '%s\n' 'Firmware build targets:'
	@printf '%s\n' '  make setup     Clone/install ESP-ADF and ESP-IDF tools'
	@printf '%s\n' '  make firmware  Build firmware into build/'
	@printf '%s\n' '  make package   Package an existing build into dist/'
	@printf '%s\n' '  make release   Build firmware and package release artifacts'
	@printf '%s\n' '  make clean     Remove build/ and dist/'

setup:
	@bash scripts/setup_firmware_env.sh

firmware:
	@bash scripts/build_firmware.sh

package:
	@python3 scripts/package_firmware.py \
		--build-dir "$(BUILD_DIR)" \
		--dist-dir "$(DIST_DIR)" \
		--project-name "$(FIRMWARE_PROJECT_NAME)" \
		--idf-target "$(IDF_TARGET)"

release: firmware
	@$(MAKE) package

clean:
	@rm -rf "$(BUILD_DIR)" "$(DIST_DIR)"
