JOBS ?= $(shell nproc)
PRESET ?= linux-default-relwithdebinfo
BUILD_DIR := build/$(PRESET)

MOD_NAME ?= hello_flecs
MOD_TARGET ?= hello_flecs
MOD_SOURCE_DIR := examples/mods/$(MOD_NAME)
MOD_OUTPUT := $(BUILD_DIR)/examples/mods/$(MOD_NAME)/$(MOD_NAME).so
MOD_INSTALL_DIR ?= $(HOME)/.local/share/TwilitRealm/Dusklight/mods/$(MOD_NAME)

.DEFAULT_GOAL := help

help:
	@echo "Dusklight dev workflow"
	@echo ""
	@echo "  make configure     Configure CMake preset $(PRESET)"
	@echo "  make game          Build only the Dusklight executable"
	@echo "  make mod           Build only the example mod .so"
	@echo "  make install-mod   Copy the example mod into the user mods folder"
	@echo "  make run           Run the current Dusklight binary"
	@echo "  make run-mod       Build/install the example mod, then run Dusklight"
	@echo "  make check-mod     Verify exported entrypoint/symbols for mod loading"
	@echo "  make why-game      Explain why Ninja wants to rebuild dusklight"
	@echo "  make repair-cache  Repair Ninja logs after interrupted builds"
	@echo "  make stop-build    Stop a stuck cmake/ninja build for this preset"
	@echo ""
	@echo "Variables: PRESET=$(PRESET) JOBS=$(JOBS) MOD_NAME=$(MOD_NAME)"

configure:
	cmake --preset $(PRESET)

game:
	cmake --build --preset $(PRESET) --target dusklight -j $(JOBS)

build: game

mod:
	cmake --build --preset $(PRESET) --target $(MOD_TARGET) -j $(JOBS)

mod-example: mod

install-mod: mod
	mkdir -p "$(MOD_INSTALL_DIR)"
	cp "$(MOD_SOURCE_DIR)/mod.json" "$(MOD_INSTALL_DIR)/"
	cp "$(MOD_OUTPUT)" "$(MOD_INSTALL_DIR)/"
	@echo "Installed $(MOD_NAME) to $(MOD_INSTALL_DIR)"

run:
	$(BUILD_DIR)/dusklight

run-mod: install-mod run

check-mod: mod
	nm -D "$(BUILD_DIR)/dusklight" | rg " ecs_progress$$| ecs_system_init$$|FLECS_IDecs_f32_tID_"
	nm -D "$(MOD_OUTPUT)" | rg "dusk_mod_init"

why-game:
	ninja -C "$(BUILD_DIR)" -d explain -n dusklight

repair-cache: stop-build
	ninja -C "$(BUILD_DIR)" -t recompact || true
	ninja -C "$(BUILD_DIR)" -t restat || true
	@echo "If the next 'make game' still rebuilds everything, let it finish once."

stop-build:
	-pkill -TERM -f "cmake --build --preset $(PRESET)"
	-pkill -TERM -f "ninja.*$(BUILD_DIR)"

clean-mod:
	rm -rf "$(MOD_INSTALL_DIR)"
	rm -f "$(MOD_OUTPUT)"

clean:
	rm -rf "$(BUILD_DIR)"

.PHONY: help configure game build mod mod-example install-mod run run-mod check-mod why-game repair-cache stop-build clean-mod clean
