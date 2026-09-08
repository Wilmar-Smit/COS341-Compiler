BUILD_DIR = build
EXEC = compiler

.PHONY: all build run clean re compile_commands valgrind

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
	@rm -rf $(BUILD_DIR) compile_commands.json .clangd .cache

re: clean build

# --- Added on: Valgrind target ---
valgrind: build
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(BUILD_DIR)/$(EXEC)


# --- Added on: Clang-Tidy Linter ---
lint: compile_commands
	@if command -v clang-tidy >/dev/null 2>&1; then \
		clang-tidy src/*.cpp -p $(BUILD_DIR) --checks='modernize-*,readability-*,performance-*'; \
	else \
		echo "clang-tidy is not installed. Install it via your package manager."; \
	fi
