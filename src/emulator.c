#include "emulator.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MEMORY_SIZE 4096  // 4096 == 0xFFF

typedef void (*OpcodeHandler)(uint16_t);

static OpcodeHandler opcode_0xxx(uint16_t opcode);
static OpcodeHandler opcode_1xxx(uint16_t opcode);
static OpcodeHandler opcode_2xxx(uint16_t opcode);
static OpcodeHandler opcode_3xxx(uint16_t opcode);
static OpcodeHandler opcode_4xxx(uint16_t opcode);
static OpcodeHandler opcode_5xxx(uint16_t opcode);
static OpcodeHandler opcode_6xxx(uint16_t opcode);
static OpcodeHandler opcode_7xxx(uint16_t opcode);
static OpcodeHandler opcode_8xxx(uint16_t opcode);
static OpcodeHandler opcode_9xxx(uint16_t opcode);
static OpcodeHandler opcode_Axxx(uint16_t opcode);
static OpcodeHandler opcode_Bxxx(uint16_t opcode);
static OpcodeHandler opcode_Cxxx(uint16_t opcode);
static OpcodeHandler opcode_Dxxx(uint16_t opcode);
static OpcodeHandler opcode_Exxx(uint16_t opcode);
static OpcodeHandler opcode_Fxxx(uint16_t opcode);

static OpcodeHandler opcodes_array[16] = {
    (OpcodeHandler)opcode_0xxx,
    (OpcodeHandler)opcode_1xxx,
    (OpcodeHandler)opcode_2xxx,
    (OpcodeHandler)opcode_3xxx,
    (OpcodeHandler)opcode_4xxx,
    (OpcodeHandler)opcode_5xxx,
    (OpcodeHandler)opcode_6xxx,
    (OpcodeHandler)opcode_7xxx,
    (OpcodeHandler)opcode_8xxx,
    (OpcodeHandler)opcode_9xxx,
    (OpcodeHandler)opcode_Axxx,
    (OpcodeHandler)opcode_Bxxx,
    (OpcodeHandler)opcode_Cxxx,
    (OpcodeHandler)opcode_Dxxx,
    (OpcodeHandler)opcode_Exxx,
    (OpcodeHandler)opcode_Fxxx
};

uint16_t I;
uint8_t V[16] = { 0 };  // Registers in 8-chip architecture

uint8_t memory[MEMORY_SIZE] = { 0 };
uint8_t stack_pointer = 0;

uint16_t program_counter = 0x200;  // 512 == 0x200

void emu_load_file(const char *filename)
{
    FILE *f;
    if ((f = fopen(filename, "rb")) == NULL)
    {
        perror("Faild to open file");
        exit(EXIT_FAILURE);
    }

    uint16_t index = program_counter;

    int b;
    while ((b = getc(f)) != EOF && index < MEMORY_SIZE)
    {
        memory[index] = b;
        index++;
    }

    fclose(f);
}

void emulate()
{
    uint8_t left_byte = memory[program_counter];
    uint8_t right_byte = memory[program_counter + 1];

    printf("left = %02X\n", left_byte);
    printf("right = %02X\n", right_byte);

    uint16_t opcode = right_byte | (left_byte << 8);  // shift by 1 byte

    printf("opcode = %04X\n\n", opcode);

    opcodes_array[(opcode & 0xF000) >> 12](opcode);
}

static OpcodeHandler opcode_0xxx(uint16_t opcode)
{
    switch (opcode)
    {
        case 0x00E0: break;  // CLS
        case 0x00EE: break;  // RET
        default:
            printf("opcode: %04X not supported\n", opcode);
            break;
    }

    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_1xxx(uint16_t opcode)
{
    // JP
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_2xxx(uint16_t opcode)
{
    // CALL
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_3xxx(uint16_t opcode)
{
    // SE skip next instruction
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_4xxx(uint16_t opcode)
{
    // SNE
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_5xxx(uint16_t opcode)
{
    // SE
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_6xxx(uint16_t opcode)
{
    // LD
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_7xxx(uint16_t opcode)
{
    // LD
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_8xxx(uint16_t opcode)
{
    // LD, OR, AND ...
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_9xxx(uint16_t opcode)
{
    // SNE
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_Axxx(uint16_t opcode)
{
    // LD I
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_Bxxx(uint16_t opcode)
{
    // JP V0
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_Cxxx(uint16_t opcode)
{
    // RND
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_Dxxx(uint16_t opcode)
{
    // DRW -> draw
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_Exxx(uint16_t opcode)
{
    // SKP -> skip
    return (OpcodeHandler)0;
}

static OpcodeHandler opcode_Fxxx(uint16_t opcode)
{
    // LD DT
    return (OpcodeHandler)0;
}
