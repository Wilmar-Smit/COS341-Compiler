BUILD_DIR = build
EXEC = compiler

.PHONY: all build run clean re compile_commands

all: build

build: compile_commands
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		cmake -B $(BUILD_DIR) -S . -G Ninja -DCMAKE_CXX_COMPILER=clang++; \
	fi
	@cmake --build $(BUILD_DIR)

compile_commands:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		cmake -B $(BUILD_DIR) -S . -G Ninja -DCMAKE_CXX_COMPILER=clang++; \
	fi
	@if [ -f "$(BUILD_DIR)/compile_commands.json" ]; then \
		ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json; \
	fi

run: build
	@./$(BUILD_DIR)/$(EXEC)

compile:
	@./$(BUILD_DIR)/$(EXEC)
clean:
	@rm -rf $(BUILD_DIR) compile_commands.json .clangd

re: clean build
