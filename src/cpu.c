#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE 16
#define MEMORY_SIZE 2048

// Define a struct for the individual flags using bitfields
typedef struct {
    bool zero : 1;     // 1 bit for zero flag
    bool carry : 1;    // 1 bit for carry flag
} Flags;

// Represents a decoded instruction.
typedef struct {
    uint16_t opcode; // Operation code
    uint8_t regA;    // First register
    uint8_t regB;    // Second register
    uint8_t regC;    // Third register
    uint8_t imm;     // Immediate value
} Instruction;

// Defines all supported opcodes.
typedef enum {
    OPCODE_NOP = 0x00,  // No operation
    OPCODE_HLT = 0x01,  // Halt execution
    OPCODE_ADD = 0x02,  // Add regA + regB -> regC
    OPCODE_SUB = 0x03,  // Subtract regB from regA -> regC
    OPCODE_NOR = 0x04,  // Bitwise NOR
    OPCODE_AND = 0x05,  // Bitwise AND
    OPCODE_XOR = 0x06,  // Bitwise XOR
    OPCODE_RSH = 0x07,  // Logical right shift
    OPCODE_LDI = 0x08,  // Load immediate into regA
    OPCODE_ADI = 0x09,  // Add immediate to regA
    OPCODE_JMP = 0x0A,  // Unconditional jump
    OPCODE_BRH = 0x0B,  // Conditional branch
    OPCODE_CAL = 0x0C,  // Call subroutine
    OPCODE_RET = 0x0D,  // Return from subroutine
    OPCODE_LOD = 0x0E,  // Load memory -> regA
    OPCODE_STR = 0x0F   // Store regA -> memory
} Opcode;

// Define the CPU struct
typedef struct {
    uint8_t registers[16];              // 16 general-purpose registers
    uint16_t program_counter;           // Program counter
    Flags flags;                        // Flags register (using the Flags struct)
    uint16_t instruction_memory[2048];  // Instruction memory
    uint16_t stack[STACK_SIZE];
    uint8_t stack_pointer;              // Points to the top of the stack
} CPU;

// Call stack related functions
void push(CPU *cpu, uint16_t value) {
    // Check if there is space left in the stack (stack pointer is within bounds)
    if (cpu->stack_pointer < STACK_SIZE) {
        // Store the value in the current position of the stack and increment the stack pointer
        cpu->stack[cpu->stack_pointer++] = value;
    }
}

uint16_t pop(CPU *cpu) {
    // Check if there is something to pop (stack pointer is greater than 0)
    if (cpu->stack_pointer > 0) {
        // Return the value at the current top of the stack and decrement the stack pointer
        return cpu->stack[--cpu->stack_pointer];
    }
    // If the stack is empty, return an error value (0xFFFF)
    return 0xFFFF;
}

/*
 * Decodes a 16-bit raw instruction into its components (opcode, registers, immediate).
 */
Instruction decode_instruction(uint16_t raw_instruction) {
    Instruction inst;

    inst.opcode = (raw_instruction >> 12) & 0x0F; // Top 4 bits: opcode
    inst.regA = (raw_instruction >> 8) & 0x0F;    // Next 4 bits: regA
    inst.regB = (raw_instruction >> 4) & 0x0F;    // Next 4 bits: regB
    inst.regC = raw_instruction & 0x0F;           // Last 4 bits: regC
    inst.imm = (uint8_t)(raw_instruction & 0xFF); // Last 8 bits: immediate value

    return inst;
}

void execute(CPU *cpu, Instruction inst) {
    cpu->registers[0] = 0;
    switch (inst.opcode) {
        case OPCODE_NOP:
            // No operation: do nothing
            cpu->program_counter++; // Increment the program counter
            break;

        case OPCODE_HLT:
            // Halt: stop the program
            break;

        case OPCODE_ADD:
            // Addition: regC = regA + regB
            {
                uint16_t result = cpu->registers[inst.regA] + cpu->registers[inst.regB];
                cpu->registers[inst.regC] = (uint8_t)result;
                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = (result > 0xFF);  // Carry flag is set if overflow occurs

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_SUB:
            // Subtraction: regC = regA - regB
            {
                int16_t result = (int8_t)cpu->registers[inst.regA] - (int8_t)cpu->registers[inst.regB];
                cpu->registers[inst.regC] = (uint8_t)result;
                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = (result < 0);  // Carry flag is set if there's a borrow

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_NOR:
            // Bitwise NOR: regC = ~(regA | regB)
            {
                cpu->registers[inst.regC] = ~(cpu->registers[inst.regA] | cpu->registers[inst.regB]);
                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = false;  // NOR doesn't affect carry

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_AND:
            // Bitwise AND: regC = regA & regB
            {
                cpu->registers[inst.regC] = cpu->registers[inst.regA] & cpu->registers[inst.regB];
                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = false;  // AND doesn't affect carry

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_XOR:
            {
                // Bitwise XOR: regC = regA ^ regB
                cpu->registers[inst.regC] = cpu->registers[inst.regA] ^ cpu->registers[inst.regB];
                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = false;  // XOR doesn't affect carry

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_RSH:
            {
                // Right Shift: regC = regA >> 1 (shift A by 1 position)
                cpu->registers[inst.regC] = cpu->registers[inst.regA] >> 1;
                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = (cpu->registers[inst.regA] & 0x01);  // Carry is the last bit shifted out

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_LDI:
            {
                // Load Immediate: regA = imm
                cpu->registers[inst.regA] = inst.imm;
                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regA] == 0);
                cpu->flags.carry = false;  // Load immediate doesn't affect carry

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_ADI:
            // Add Immediate: regA = regA + imm
            {
                uint16_t result = cpu->registers[inst.regA] + inst.imm;
                cpu->registers[inst.regA] = (uint8_t)result;
                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regA] == 0);
                cpu->flags.carry = (result > 0xFF);  // Carry flag if overflow

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_JMP:
            // Jump: set the program counter to the immediate value (jump to address)
            cpu->program_counter = inst.imm;
            break;

        case OPCODE_BRH:
            // Branch: if the zero flag is set, jump to the immediate address
            if (cpu->flags.zero) {  // Check zero flag
                cpu->program_counter = inst.imm;
            }
            break;

        case OPCODE_CAL:
            // Push the current program counter (PC) onto the stack
            push(cpu, cpu->program_counter);
            // Set the program counter to the target address (function call)
            cpu->program_counter = inst.imm;
            break;

        case OPCODE_RET:
            // Pop the return address from the stack and set it to the program counter
            cpu->program_counter = pop(cpu);
            break;

        case OPCODE_LOD:
            // Memory Load: regA = memory[regB]
            cpu->registers[inst.regA] = cpu->instruction_memory[cpu->registers[inst.regB]];

            cpu->program_counter++; // Increment the program counter
            break;

        case OPCODE_STR:
            // Memory Store: memory[regB] = regA
            cpu->instruction_memory[cpu->registers[inst.regB]] = cpu->registers[inst.regA];

            cpu->program_counter++; // Increment the program counter
            break;

        default:
            // Invalid opcode, handle error if necessary
            break;
    }
    cpu->registers[0] = 0;
}

void run(CPU *cpu){
    while (true) {
        // Fetch instruction from memory using the program counter
        uint16_t raw_instruction = cpu->instruction_memory[cpu->program_counter];

        // Decode the instruction into its components
        Instruction inst = decode_instruction(raw_instruction);

        // Execute the instruction
        execute(cpu, inst);

        // If the opcode is HALT, break the loop and stop execution
        if (inst.opcode == OPCODE_HLT) {
            break;
        }
    }
}


// DEBUGGING PURPOSES ONLY, TEMPORARY FUNCTION
void load_program(const char *filename, CPU *cpu) {
    // Open the file in text read mode
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return;
    }

    char line[33];  // Max length for a 16-bit binary number string (plus null terminator)
    size_t i = 0;
    
    // Read the file line by line
    while (fgets(line, sizeof(line), file) != NULL && i < MEMORY_SIZE) {
        // Remove newline character from the line (if any)
        line[strcspn(line, "\n")] = '\0';

        // Convert binary string to uint16_t (base 2)
        uint16_t instruction = (uint16_t)strtol(line, NULL, 2);

        // Store the instruction in memory
        cpu->instruction_memory[i] = instruction;
        i++;
    }

    // Check if we read the full program or encountered an error
    if (i == MEMORY_SIZE) {
        printf("Warning: Memory is full, the program might be truncated.\n");
    }

    // Close the file after reading
    fclose(file);

    // Print success message
    printf("Program loaded successfully from %s\n", filename);
}


// DEBUGGING PURPOSES ONLY, TEMPORARY FUNCTION
void print_registers(CPU *cpu) {
    printf("Registers:\n");
    for (int i = 0; i < 16; i++) {
        printf("R%d: %d\n", i, cpu->registers[i]);  // Print register value in decimal
    }
}


int main() {
    CPU cpu = {0};

    load_program("./build/program.bin", &cpu);

    run(&cpu);

    print_registers(&cpu);

    return 0;
}