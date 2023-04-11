.PHONY: help
help:
	@echo "usage: make [target]"
	@echo
	@echo "targets:"
	@echo "  help                          show this message"
	@echo "  build                         builds fastchess"
	@echo "  run                           runs fastchess"
	@echo "  test                          runs tests"
	@echo "  pull-submodules               pull all git submodules"
	@echo

.PHONY: build
build:
	# Building chip8 ...
	@mkdir -p build
	@cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -S . -B build
	@make -C build

.PHONY: run
run:
	# Running chip8 ...
	@\
  ./build/chip8
	# OK

.PHONY: pull-submodules
pull-submodules:
	# Pulling sumbodules ...
	@git submodule update --init
	# OK

.PHONY: test
test:
	# Running tests ...
	@CTEST_OUTPUT_ON_FAILURE=1 make -C build/test test
