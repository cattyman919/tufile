CXX=clang++

TARGET_GENERATOR=Ninja

PROGRAM_NAME=template

BUILD_DIRECTORY=build
BUILD_SYSTEM=ninja
BUILD_SYSTEM_FLAGS=-C $(BUILD_DIRECTORY) --quiet

CXXFLAGS=

.PHONY: build
build:
	@cmake -B build -G $(TARGET_GENERATOR) -DPROGRAM_NAME=$(PROGRAM_NAME)

.PHONY: run
run: build
	@$(BUILD_SYSTEM) $(BUILD_SYSTEM_FLAGS)
	@./$(BUILD_DIRECTORY)/$(PROGRAM_NAME)

.PHONY: clean
clean:
	rm -rf build/

print: $(wildcard *.cpp)
	@echo $?
	@ls -la  $?
