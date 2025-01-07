#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/cpu.h"

// Push instruction memory address into the call stack
void push_stack(CPU *cpu, uint16_t value) {
    // Check if there is space left in the stack (stack pointer is within bounds)
    if (cpu->stack_pointer < STACK_SIZE) {
        // Store the value in the current position of the stack and increment the stack pointer
        cpu->stack[cpu->stack_pointer++] = value;
    }
}

// Pop last address from the call stack
uint16_t pop_stack(CPU *cpu) {
    // Check if there is something to pop (stack pointer is greater than 0)
    if (cpu->stack_pointer > 0) {
        // Return the value at the current top of the stack and decrement the stack pointer
        return cpu->stack[--cpu->stack_pointer]+1;
    }
    // If the stack is empty, return an error value (0xFFFF)
    return 0xFFFF;
}

// Write value into a register
void write(CPU *cpu, uint8_t index, uint8_t value) {
    index &= 0x0F; // Make sure index doesn't exceed 4 bits (16 total addresses)
    cpu->registers[index] = value; // Asing value to the register
}

// Read value from register
uint8_t read(CPU *cpu, uint8_t index) {
    index &= 0x0F; // Make sure index doesn't exceed 4 bits (16 total addresses)
    return cpu->registers[index];
}

/*
 * Decodes a 16-bit raw instruction into its components (opcode, registers, immediate, condition and address).
 */
Instruction decode_instruction(uint16_t raw_instruction) {
    Instruction inst;

    inst.opcode = (raw_instruction >> 12) & 0x0F; // pos: 1111 0000 0000 0000 Operation Code
    inst.regA = (raw_instruction >> 8) & 0x0F;    // pos: 0000 1111 0000 0000 Register A
    inst.regB = (raw_instruction >> 4) & 0x0F;    // pos: 0000 0000 1111 0000 Register B
    inst.regC = raw_instruction & 0x0F;           // pos: 0000 0000 0000 1111 Register C
    inst.imm = (uint8_t)(raw_instruction & 0xFF); // pos: 0000 0000 1111 1111 Immediate
    inst.cond = (raw_instruction >> 10) & 0x03;   // pos: 0000 1100 0000 0000 Branch condition
    inst.address = raw_instruction & 0x03FF;      // pos: 0000 0011 1111 1111 Instruction address

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
                uint16_t result = read(cpu, inst.regA) + read(cpu, inst.regB);
                write(cpu, inst.regC, (uint8_t)result);

                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = (result > 0xFF);  // Carry flag is set if overflow occurs

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_SUB:
            // Subtraction: regC = regA - regB
            {
                uint16_t result = read(cpu, inst.regA) - read(cpu, inst.regB);
                write(cpu, inst.regC, (uint8_t)result);

                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = (result < 0);  // Carry flag is set if there's a borrow

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_NOR:
            // Bitwise NOR: regC = ~(regA | regB)
            {
                uint8_t result = ~(read(cpu, inst.regA) | read(cpu, inst.regB));
                write(cpu, inst.regC, result);

                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = false;  // NOR doesn't affect carry

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_AND:
            // Bitwise AND: regC = regA & regB
            {
                uint8_t result = read(cpu, inst.regA) & read(cpu, inst.regB);
                write(cpu, inst.regC, result);

                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = false;  // AND doesn't affect carry

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_XOR:
            {
                // Bitwise XOR: regC = regA ^ regB
                uint8_t result = read(cpu, inst.regA) ^ read(cpu, inst.regB);
                write(cpu, inst.regC, result);

                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = false;  // XOR doesn't affect carry

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_RSH:
            {
                // Right Shift: regC = regA >> 1 (shift A by 1 position)
                uint8_t result = read(cpu, inst.regA) >> 1;
                write(cpu, inst.regC, result);

                // Update flags
                cpu->flags.zero = (cpu->registers[inst.regC] == 0);
                cpu->flags.carry = (cpu->registers[inst.regA] & 0x01);  // Carry is the last bit shifted out

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_LDI:
            {
                // Load Immediate: regA = imm
                write(cpu, inst.regA, inst.imm);

                // Update flags
                cpu->flags.zero = false; // Load immediate doesn't affect zero
                cpu->flags.carry = false;  // Load immediate doesn't affect carry

                cpu->program_counter++; // Increment the program counter
            }
            break;

        case OPCODE_ADI:
            // Add Immediate: regA = regA + imm
            {
                uint16_t result = read(cpu, inst.regA) + inst.imm;
                write(cpu, inst.regA, (uint8_t)result);

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
            // Branch operation: conditionally update the program counter based on the flag status
            {
                // Evaluate the condition specified in the instruction
                switch (inst.cond) {
                    case ZERO_TRUE:
                        // Jump if the zero flag is set
                        if (cpu->flags.zero) {
                            cpu->program_counter = inst.address;  // Set program counter to the specified address
                        } else {
                            cpu->program_counter++; // Increment the program counter
                        }
                        break;

                    case ZERO_FALSE:
                        // Jump if the zero flag is not set
                        if (!cpu->flags.zero) {
                            cpu->program_counter = inst.address;  // Set program counter to the specified address
                        } else {
                            cpu->program_counter++; // Increment the program counter
                        }
                        break;

                    case CARRY_TRUE:
                        // Jump if the carry flag is set
                        if (cpu->flags.carry) {
                            cpu->program_counter = inst.address;  // Set program counter to the specified address
                        } else {
                            cpu->program_counter++; // Increment the program counter
                        }
                        break;

                    case CARRY_FALSE:
                        // Jump if the carry flag is not set
                        if (!cpu->flags.carry) {
                            cpu->program_counter = inst.address;  // Set program counter to the specified address
                        } else {
                            cpu->program_counter++; // Increment the program counter
                        }
                        break;

                    default:
                        // Invalid condition: no action taken
                        break;
                }
            }
            break;

        case OPCODE_CAL:
            // Push the current program counter (PC) onto the stack
            push_stack(cpu, cpu->program_counter);
            // Set the program counter to the target address (function call)
            cpu->program_counter = inst.imm;
            break;

        case OPCODE_RET:
            // Pop the return address from the stack add one and set it to the program counter
            cpu->program_counter = pop_stack(cpu);
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
        printf("R%d: %d\n", i, read(cpu, i));  // Print register value in decimal
    }
}

// DEBUGGING PURPOSES ONLY, TEMPORARY FUNCTION
void print_flags(CPU *cpu) {
    printf("zero: %d\n", cpu->flags.zero);
    printf("carry: %d\n", cpu->flags.carry);
}


int main() {
    CPU cpu = {0};

    load_program("./build/program.bin", &cpu);

    run(&cpu);

    print_registers(&cpu);
    print_flags(&cpu);

    return 0;
}