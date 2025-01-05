# Paths
SRC_DIR := src
ASM_DIR := assembler
BUILD_DIR := build
BIN_DIR := bin

# Files
ASM_SCRIPT := $(ASM_DIR)/assembler.py
ASM_INPUT := program.as
ASM_OUTPUT := $(BUILD_DIR)/program.bin
SRC_FILE := $(SRC_DIR)/cpu.c
EXE := $(BIN_DIR)/emulator

# Default rule
all: assemble compile run

# Step 1: Assemble the program
assemble: $(ASM_OUTPUT)

$(ASM_OUTPUT): $(ASM_SCRIPT) $(ASM_INPUT)
	mkdir -p $(BUILD_DIR)
	python3 $(ASM_SCRIPT) $(ASM_INPUT) $(ASM_OUTPUT)

# Step 2: Compile the CPU emulator
compile: $(EXE)

$(EXE): $(SRC_FILE)
	mkdir -p $(BIN_DIR)
	gcc -g $(SRC_FILE) -o $(EXE)

# Step 3: Run the emulator
run:
	./$(EXE)

# Clean up build and bin directories
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all assemble compile run clean
