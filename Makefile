PORT ?= /dev/ttyACM0
BADGELINKPORT ?= $(PORT)

IDF_PATH ?= $(shell cat .IDF_PATH 2>/dev/null || echo `pwd`/esp-idf)
IDF_TOOLS_PATH ?= $(shell cat .IDF_TOOLS_PATH 2>/dev/null || echo `pwd`/esp-idf-tools)
IDF_BRANCH ?= v5.5.1
#IDF_COMMIT ?= aaebc374676621980878789c49d239232ea714c5
IDF_EXPORT_QUIET ?= 1
IDF_GITHUB_ASSETS ?= dl.espressif.com/github_assets
MAKEFLAGS += --silent

SHELL := /usr/bin/env bash

DEVICE ?= tanmatsu
BUILD ?= build/$(DEVICE)
FAT ?= 0
SDKCONFIG_DEFAULTS ?= sdkconfigs/general;sdkconfigs/$(DEVICE)
SDKCONFIG ?= sdkconfig_$(DEVICE)

####

# Set IDF_TARGET based on device name

ifeq ($(DEVICE), tanmatsu)
IDF_TARGET ?= esp32p4
else ifeq ($(DEVICE), konsool)
IDF_TARGET ?= esp32p4
else ifeq ($(DEVICE), esp32-p4-function-ev-board)
IDF_TARGET ?= esp32p4
else ifeq ($(DEVICE), mch2022)
IDF_TARGET ?= esp32
else ifeq ($(DEVICE), kami)
IDF_TARGET ?= esp32
else ifeq ($(DEVICE), hackerhotel-2024)
IDF_TARGET ?= esp32c6
else
$(warning "Unknown device, defaulting to ESP32 $(DEVICE)")
IDF_TARGET ?= esp32
endif

IDF_PARAMS := -B $(BUILD) build -DDEVICE=$(DEVICE) -DSDKCONFIG_DEFAULTS="$(SDKCONFIG_DEFAULTS)" -DSDKCONFIG=$(SDKCONFIG) -DIDF_TARGET=$(IDF_TARGET) -DFAT=$(FAT)

#####

export IDF_TOOLS_PATH
export IDF_GITHUB_ASSETS

# General targets

.PHONY: all
all: build

# Badgelink

# Determine badgelink connection argument: --tcp for host:port, --port for serial devices
BADGELINK_CONN := $(if $(findstring :,$(BADGELINKPORT)),--tcp $(BADGELINKPORT),--port $(BADGELINKPORT))

.PHONY: badgelink
badgelink:
	rm -rf badgelink
	#git clone https://github.com/badgeteam/esp32-component-badgelink.git badgelink
	git clone https:///github.com/nullislandspace/esp32-component-badgelink.git badgelink
	cd badgelink/tools; ./install.sh

APP_SLUG ?= at.cavac.blinkensisters
# Kept in step with VERSION in main/shared/globals.h, which names the data dir.
GAME_VERSION := $(shell sed -n 's/^#define VERSION "\(.*\)"/\1/p' main/shared/globals.h)
APP_INSTALL_BASE_PATH ?= /sd/apps/
APP_INSTALL_PATH = $(APP_INSTALL_BASE_PATH)$(APP_SLUG)

.PHONY: install
install: build
	@echo "=== Installing application ==="
	@echo "Uploading application.bin to AppFS as $(APP_SLUG)..."
	cd badgelink/tools; ./badgelink.sh $(BADGELINK_CONN) appfs upload $(APP_SLUG) "BlinkenSisters" 0 ../../$(BUILD)/tanmatsu-blinkensisters.bin
	@echo "Creating directory $(APP_INSTALL_PATH)..."
	cd badgelink/tools; ./badgelink.sh $(BADGELINK_CONN) fs mkdir $(APP_INSTALL_PATH) || true
	@echo "Uploading metadata.json..."
	cd badgelink/tools; ./badgelink.sh $(BADGELINK_CONN) fs upload $(APP_INSTALL_PATH)/metadata.json ../../metadata/metadata.json
	@echo "Uploading icon16.png..."
	cd badgelink/tools; ./badgelink.sh $(BADGELINK_CONN) fs upload $(APP_INSTALL_PATH)/icon16.png ../../metadata/icon16.png
	@echo "Uploading icon32.png..."
	cd badgelink/tools; ./badgelink.sh $(BADGELINK_CONN) fs upload $(APP_INSTALL_PATH)/icon32.png ../../metadata/icon32.png
	@echo "Uploading icon64.png..."
	cd badgelink/tools; ./badgelink.sh $(BADGELINK_CONN) fs upload $(APP_INSTALL_PATH)/icon64.png ../../metadata/icon64.png
	@echo "=== Installation complete ==="
	@echo "(The game downloads its data itself; see 'make publishaddons'.)"

# Development only. Players get the game data from in-game downloads (published
# with 'make publishaddons'); this uploads archives from sdcard/ straight into
# the app's folder instead, to try one before publishing it -- the game still
# unpacks archives found there, and a downloaded copy of the same name wins.
# Uploads everything (~80 MB, minutes) unless BMF names some:
#   make installbmf BMF="mz_xmas2007"
#   make installbmf BMF="basedata LostPixels"
.PHONY: installbmf
installbmf:
	@echo "=== Uploading archives for testing ==="
	cd badgelink/tools; ./badgelink.sh $(BADGELINK_CONN) fs mkdir $(APP_INSTALL_PATH) || true
	cd badgelink/tools; ./badgelink.sh $(BADGELINK_CONN) fs mkdir $(APP_INSTALL_PATH)/addons || true
	cd badgelink/tools; for f in $(if $(BMF),$(foreach b,$(BMF),$(if $(filter basedata,$(b)),../../sdcard/basedata.bmf,../../sdcard/addons/$(b).bmf)),../../sdcard/basedata.bmf ../../sdcard/addons/*.bmf); do \
		test -f $$f || { echo "no such archive: $$f"; exit 1; }; \
		case $$(basename $$f) in basedata.bmf) dest=$(APP_INSTALL_PATH)/basedata.bmf;; *) dest=$(APP_INSTALL_PATH)/addons/$$(basename $$f);; esac; \
		echo "  $$(basename $$f)"; \
		./badgelink.sh $(BADGELINK_CONN) fs upload $$dest $$f || exit 1; \
	done
	@echo "=== Archives uploaded ==="

# The game stamps each archive it unpacks with its size, time and CRC32, and on
# launch unpacks only the ones that no longer match -- so after installbmf there
# is nothing to do. This is for forcing it anyway: it deletes every stamp, and
# the next launch unpacks everything (which takes a few minutes on the device).
.PHONY: resetdata
resetdata:
	@echo "=== Forcing re-extraction on next launch ==="
	cd badgelink/tools; for f in basedata.bmf $$(cd ../../sdcard/addons && ls *.bmf); do \
		./badgelink.sh $(BADGELINK_CONN) fs delete /sd/blinkensisters/V$(GAME_VERSION)/.extracted_$$f || true; \
	done

# Addon downloads: publish changed archives as GitHub releases and update
# addons/index.json (see tools/publish-addons.py). Prints the plan unless
# PUBLISH=1; pass --add/--remove through ADDONARGS.
#   make publishaddons
#   make publishaddons PUBLISH=1
#   make publishaddons PUBLISH=1 ADDONARGS="--add sdcard/addons/icy.bmf"
.PHONY: publishaddons
publishaddons:
	@$(MAKE) -C tools >/dev/null
	tools/publish-addons.py $(if $(PUBLISH),--publish) $(ADDONARGS)

.PHONY: run
run:
	cd badgelink/tools; ./badgelink.sh $(BADGELINK_CONN) start $(APP_SLUG)

# App repository

APP_REPO_PATH ?= ../tanmatsu-app-repository/$(APP_SLUG)

# Stage everything metadata.json promises into the app repository. The asset
# paths have to match its source_file entries exactly, including the addons/
# subdirectory -- the repository keeps that layout (see nl.mansoft.mqtt), and
# the launcher creates the directories on install.
#
# The data copy is driven BY metadata.json rather than by a glob over sdcard/,
# so metadata.json is the single thing that decides what ships. An archive can
# then live in sdcard/ without being published -- mz_xmas2007 does -- and
# dropping an entry is enough to withdraw it. A glob would quietly publish
# whatever happened to be lying in the directory.
#
# Note the rename: the build produces tanmatsu-blinkensisters.bin, but the
# repository entry is the "executable" metadata.json names, application.bin.
.PHONY: apprepo
apprepo: build
	@echo "=== Updating app repository ==="
	mkdir -p $(APP_REPO_PATH)
	cp metadata/metadata.json $(APP_REPO_PATH)/metadata.json
	cp metadata/icon16.png $(APP_REPO_PATH)/icon16.png
	cp metadata/icon32.png $(APP_REPO_PATH)/icon32.png
	cp metadata/icon64.png $(APP_REPO_PATH)/icon64.png
	cp $(BUILD)/tanmatsu-blinkensisters.bin $(APP_REPO_PATH)/application.bin
	@# The game data is no longer part of the app: it downloads in-game. Any
	@# asset metadata.json does not declare would ship to nobody, so remove it.
	@python3 -c "import json,os,glob; p='$(APP_REPO_PATH)'; a=json.load(open('metadata/metadata.json'))['application'][0]; keep=set(x['source_file'] for x in a.get('assets',[])) | {a['executable'],'metadata.json','icon16.png','icon32.png','icon64.png'}; stray=sorted(os.path.relpath(f,p) for f in glob.glob(p+'/**/*',recursive=True) if os.path.isfile(f) and os.path.relpath(f,p) not in keep); [print('  removing undeclared '+f) or os.remove(os.path.join(p,f)) for f in stray]; [os.rmdir(d) for d in sorted(glob.glob(p+'/**/',recursive=True),reverse=True) if d.rstrip('/')!=p and not os.listdir(d)]"
	@python3 -c "import json,os,sys; p='$(APP_REPO_PATH)'; a=json.load(open('metadata/metadata.json'))['application'][0]; missing=[x['source_file'] for x in a.get('assets',[]) if not os.path.isfile(os.path.join(p,x['source_file']))]; missing += [f for f in [a['executable'],'metadata.json','icon16.png','icon32.png','icon64.png'] if not os.path.isfile(os.path.join(p,f))]; sys.exit('MISSING in repo: '+', '.join(missing)) if missing else print('  executable, metadata and icons present')"
	@echo "=== App repository updated at $(APP_REPO_PATH) ==="

# Preparation

.PHONY: prepare
prepare: submodules sdk

.PHONY: submodules
submodules: 
	if [ ! -f .submodules_update_done ]; then \
		echo "Updating submodules"; \
		git submodule update --init --recursive; \
		touch .submodules_update_done; \
	fi

.PHONY: sdk
sdk:
	if test -d "$(IDF_PATH)"; then echo -e "ESP-IDF target folder exists!\r\nPlease remove the folder or un-set the environment variable."; exit 1; fi
	if test -d "$(IDF_TOOLS_PATH)"; then echo -e "ESP-IDF tools target folder exists!\r\nPlease remove the folder or un-set the environment variable."; exit 1; fi
	git clone --recursive --branch "$(IDF_BRANCH)" https://github.com/espressif/esp-idf.git "$(IDF_PATH)" --depth=1 --shallow-submodules
#	cd "$(IDF_PATH)"; git fetch origin "$(IDF_COMMIT)" --recurse-submodules || true
#	cd "$(IDF_PATH)"; git checkout "$(IDF_COMMIT)"
	cd "$(IDF_PATH)"; git submodule update --init --recursive
	cd "$(IDF_PATH)"; bash install.sh all

.PHONY: reinstallsdk
reinstallsdk:
	cd "$(IDF_PATH)"; bash install.sh all

.PHONY: removesdk
removesdk:
	rm -rf "$(IDF_PATH)"
	rm -rf "$(IDF_TOOLS_PATH)"

.PHONY: refreshsdk
refreshsdk: removesdk sdk

.PHONY: menuconfig
menuconfig:
	source "$(IDF_PATH)/export.sh" && idf.py menuconfig -DDEVICE=$(DEVICE) -DSDKCONFIG_DEFAULTS="$(SDKCONFIG_DEFAULTS)" -DSDKCONFIG=$(SDKCONFIG) -DIDF_TARGET=$(IDF_TARGET)
	
# Cleaning

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -f .submodules_update_done

.PHONY: fullclean
fullclean: clean
	rm -f sdkconfig
	rm -f sdkconfig.old
	rm -f sdkconfig.ci
	rm -f sdkconfig.defaults

# Check if build environment is set up correctly
.PHONY: checkbuildenv
checkbuildenv:
	if [ -z "$(IDF_PATH)" ]; then echo "IDF_PATH is not set!"; exit 1; fi
	if [ -z "$(IDF_TOOLS_PATH)" ]; then echo "IDF_TOOLS_PATH is not set!"; exit 1; fi
	# Check if the IDF commit id the one we need
	#if [ -d "$(IDF_PATH)" ]; then \
	#	if [ "$(IDF_COMMIT)" != "$(shell cd $(IDF_PATH); git rev-parse HEAD)" ]; then \
	#		echo "ESP-IDF commit id does not match! Expected '$(IDF_COMMIT)' got '$(shell git rev-parse HEAD)'"; \
	#		echo "Run $ make refreshsdk"; \
	#		echo "To update the ESP-IDF to the correct commit id"; \
	#		echo "Or set the IDF_COMMIT variable in the Makefile to the correct commit id"; \
	#		exit 1; \
	#	fi; \
	#fi

# Building

.PHONY: build
build: icons checkbuildenv submodules
	source "$(IDF_PATH)/export.sh" >/dev/null && idf.py $(IDF_PARAMS)

# Hardware

.PHONY: flash
flash: build
	source "$(IDF_PATH)/export.sh" && \
	idf.py $(IDF_PARAMS) flash -p $(PORT)

.PHONY: flashmonitor
flashmonitor: build
	source "$(IDF_PATH)/export.sh" && \
	idf.py $(IDF_PARAMS) flash -p $(PORT) monitor

.PHONY: prepappfs
prepappfs:
	source "$(IDF_PATH)/export.sh" && \
	python3 managed_components/badgeteam__appfs/tools/appfs_generate.py \
	8192000 \
	appfs.bin

.PHONY: appfs
appfs:
	source "$(IDF_PATH)/export.sh" && \
	esptool.py \
		-b 921600 --port $(PORT) \
		write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB \
		0x330000 appfs.bin

.PHONY: erase
erase:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) erase-flash -p $(PORT)

.PHONY: monitor
monitor:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) monitor -p $(PORT)

.PHONY: openocd
openocd:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) openocd

.PHONY: openocdftdi
openocdftdi:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) openocd --openocd-commands "-f board/esp32p4-ftdi.cfg"

.PHONY: gdb
gdb:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) gdb

.PHONY: gdbgui
gdbgui:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) gdbgui

.PHONY: gdbtui
gdbtui:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) gdbtui

# Tools

.PHONY: size
size:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) size

.PHONY: size-components
size-components:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) size-components

.PHONY: size-files
size-files:
	source "$(IDF_PATH)/export.sh" && idf.py $(IDF_PARAMS) size-files

.PHONY: efuse
efuse:
	$(IDF_PATH)/components/efuse/efuse_table_gen.py --idf_target esp32p4 $(IDF_PATH)/components/efuse/esp32p4/esp_efuse_table.csv main/esp_efuse_custom_table.csv

# Formatting

.PHONY: format
format:
	find main/ -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' | xargs clang-format -i

# Re-compile protobuf files
# If you are an end user, you do not need to run this;
# the output files are already there in the repository.

.PHONY: compile-protobuf
compile-protobuf:
	protoc --pyi_out=tools --python_out=tools badgelink.proto
	python3 main/badgelink/nanopb/generator/nanopb_generator.py -D main/badgelink -f badgelink.options badgelink.proto

# Take all svg files from main/static/icons and put them in main/fat/icons as png using tools/connvert.sh
ICONS_SRC := $(wildcard main/static/icons/*.svg)
ICONS_DST := $(patsubst main/static/icons/%.svg,main/fat/icons/%.png,$(ICONS_SRC))

.PHONY: icons
icons: $(ICONS_DST)

main/fat/icons/%.png: main/static/icons/%.svg
	mkdir -p main/fat/icons
	tools/convert.sh $< $@
	
# Build all targets
.PHONY: buildall
buildall:
	$(MAKE) build DEVICE=tanmatsu
	$(MAKE) build DEVICE=konsool
	$(MAKE) build DEVICE=hackerhotel-2026
	$(MAKE) build DEVICE=esp32-p4-function-ev-board
	$(MAKE) build DEVICE=mch2022
	$(MAKE) build DEVICE=hackerhotel-2024

# Flash all: assumes Tanmatsu P4 is /dev/ttyACM0, C6 is /dev/ttyACM1 and MCH2022 badge is /dev/ttyACM2
.PHONY: flashall
flashall:
	$(MAKE) flash DEVICE=tanmatsu PORT=/dev/ttyACM0
	$(MAKE) flash DEVICE=mch2022 PORT=/dev/ttyACM2

# Vscode
.PHONY: vscode
vscode:
	rm -rf .vscode
	cp -r .vscode.template .vscode
