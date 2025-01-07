#ifndef CPU_H
#define CPU_H

#define STACK_SIZE 16
#define MEMORY_SIZE 2048

// Define a struct for the individual flags using bitfields
typedef struct {
    bool zero : 1;     // 1 bit for zero flag
    bool carry : 1;    // 1 bit for carry flag
} Flags;

// Represents a decoded instruction.
typedef struct {
    unsigned int opcode : 4;   // Operation code (4 bits)
    unsigned int regA : 4;     // First register (4 bits)
    unsigned int regB : 4;     // Second register (4 bits)
    unsigned int regC : 4;     // Third register (4 bits) *used as Offset when LOD/STR
    uint8_t imm;               // Immediate value (8 bits)
    unsigned int cond : 2;     // Branch condition (2 bits)
    unsigned int address : 10; // Instruction address (Instruction Memory) (10 bits)
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
    // OPCODE_LOD = 0x0E,  // Load memory -> regA
    // OPCODE_STR = 0x0F   // Store regA -> memory
} Opcode;

// Branch conditions
typedef enum {
    ZERO_TRUE = 0x00, 
    ZERO_FALSE = 0x01,
    CARRY_TRUE = 0x02,
    CARRY_FALSE = 0x03
} Condition;

// Define the CPU struct
typedef struct {
    uint8_t registers[16];              // 16 general-purpose registers
    uint16_t program_counter;           // Program counter
    Flags flags;                        // Flags register (using the Flags struct)
    uint16_t instruction_memory[2048];  // Instruction memory
    uint16_t stack[STACK_SIZE];
    uint8_t stack_pointer;              // Points to the top of the stack
} CPU;

void push(CPU *cpu, uint16_t value);
uint16_t pop(CPU *cpu);
Instruction decode_instruction(uint16_t raw_instruction);
void execute(CPU *cpu, Instruction inst);
void run(CPU *cpu);
void load_program(const char *filename, CPU *cpu);

#endif
