# Paths
SRC_DIR := src
ASM_DIR := .
BUILD_DIR := build
BIN_DIR := bin

# Files
ASM_SCRIPT := $(ASM_DIR)/assembler.py
ASM_INPUT := program.as
ASM_OUTPUT := $(BUILD_DIR)/program.bin
SRC_FILES := $(SRC_DIR)/cpu.c
EXE := $(BIN_DIR)/vmachine

# Default rule (build everything and run)
all: assemble compile run

# Step 1: Assemble the program (program.as to program.bin)
assemble: $(ASM_OUTPUT)

$(ASM_OUTPUT): $(ASM_SCRIPT) $(ASM_INPUT)
	mkdir -p $(BUILD_DIR)
	python3 $(ASM_SCRIPT) $(ASM_INPUT) $(ASM_OUTPUT)

# Step 2: Compile all source files into vmachine
compile: $(EXE)

$(EXE): $(SRC_FILES)
	mkdir -p $(BIN_DIR)
	gcc -g $(SRC_FILES) -o $(EXE)

# Step 3: Run the emulator
run: $(EXE)
	./$(EXE)

# Clean up build and bin directories
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Full build (assemble + compile + run)
full: assemble compile run

.PHONY: all assemble compile run clean full