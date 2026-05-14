#include "emulator.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define GET_NNN(x) (x & 0x0FFF)
#define GET_VX(x)  ((x & 0x0F00) >> 8)
#define GET_VY(x)  ((x & 0x00F0) >> 4)
#define GET_KK(x)  (x & 0x00FF)

#define PUSH_STACK(sp, n) (*((sp)++) = (n))  // Push on stack
#define POP_STACK(sp) (*--(sp))              // Pop from stack

#define MEMORY_SIZE 4096  // 4096 == 0xFFF

typedef void (*OpcodeHandler)(uint16_t);

static void opcode_0xxx(uint16_t opcode);
static void opcode_1xxx(uint16_t opcode);
static void opcode_2xxx(uint16_t opcode);
static void opcode_3xxx(uint16_t opcode);
static void opcode_4xxx(uint16_t opcode);
static void opcode_5xxx(uint16_t opcode);
static void opcode_6xxx(uint16_t opcode);
static void opcode_7xxx(uint16_t opcode);
static void opcode_8xxx(uint16_t opcode);
static void opcode_9xxx(uint16_t opcode);
static void opcode_Axxx(uint16_t opcode);
static void opcode_Bxxx(uint16_t opcode);
static void opcode_Cxxx(uint16_t opcode);
static void opcode_Dxxx(uint16_t opcode);
static void opcode_Exxx(uint16_t opcode);
static void opcode_Fxxx(uint16_t opcode);

static OpcodeHandler opcodes_array[16] = {
    opcode_0xxx,
    opcode_1xxx,
    opcode_2xxx,
    opcode_3xxx,
    opcode_4xxx,
    opcode_5xxx,
    opcode_6xxx,
    opcode_7xxx,
    opcode_8xxx,
    opcode_9xxx,
    opcode_Axxx,
    opcode_Bxxx,
    opcode_Cxxx,
    opcode_Dxxx,
    opcode_Exxx,
    opcode_Fxxx
};

uint16_t I;
uint8_t V[16] = { 0 };  // Registers in 8-chip architecture

uint8_t memory[MEMORY_SIZE] = { 0 };

uint16_t stack[64];
uint16_t *stack_pointer = stack;

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
    while (program_counter < MEMORY_SIZE)
    {
        uint8_t left_byte = memory[program_counter];
        uint8_t right_byte = memory[program_counter + 1];

        printf("left = %02X\n", left_byte);
        printf("right = %02X\n", right_byte);

        uint16_t opcode = right_byte | (left_byte << 8);  // shift by 1 byte

        printf("opcode = %04X\n\n", opcode);

        opcodes_array[(opcode & 0xF000) >> 12](opcode);
    }
}

static void opcode_0xxx(uint16_t opcode)
{
    switch (opcode)
    {
        case 0x00E0: break;  // CLS
        case 0x00EE: break;  // RET
        default:
            printf("opcode: %04X not supported\n", opcode);
            break;
    }
}

static void opcode_1xxx(uint16_t opcode) // JUMP
{
    program_counter = GET_NNN(opcode);
}

static void opcode_2xxx(uint16_t opcode) // CALL
{
    PUSH_STACK(stack_pointer, program_counter);
    program_counter = GET_NNN(opcode);
}

static void opcode_3xxx(uint16_t opcode) // SE Vx == kk
{
    uint8_t Vx = GET_VX(opcode);

    if (V[Vx] == GET_KK(opcode)) program_counter += 4;
    else program_counter += 2;
}

static void opcode_4xxx(uint16_t opcode) // SNE Vx != kk
{
    uint8_t Vx = GET_VX(opcode);

    if (V[Vx] != GET_KK(opcode)) program_counter += 4;
    else program_counter += 2;
}

static void opcode_5xxx(uint16_t opcode) // SE Vx == Vy
{
    if ((opcode & 0xF00F) == 0x5000)
    {
        uint8_t Vx = GET_VX(opcode);
        uint8_t Vy = GET_VY(opcode);

        if (V[Vx] == V[Vy]) program_counter += 4;
        else program_counter += 2;
    }
    else
    {
        printf("opcode: %04x not supported\n", opcode);
        exit(EXIT_FAILURE);
    }
}

static void opcode_6xxx(uint16_t opcode) // LD Vx = kk
{
    uint8_t Vx = GET_VX(opcode);

    V[Vx] = GET_KK(opcode);

    program_counter += 2;
}

static void opcode_7xxx(uint16_t opcode) // ADD Vx = Vx + kk
{
    uint8_t Vx = GET_VX(opcode);

    V[Vx] += GET_KK(opcode);
}

static void opcode_8xxx(uint16_t opcode)
{
    // LD, OR, AND ...

}

static void opcode_9xxx(uint16_t opcode)
{
    // SNE

}

static void opcode_Axxx(uint16_t opcode)
{
    // LD I

}

static void opcode_Bxxx(uint16_t opcode)
{
    // JP V0

}

static void opcode_Cxxx(uint16_t opcode)
{
    // RND

}

static void opcode_Dxxx(uint16_t opcode)
{
    // DRW -> draw

}

static void opcode_Exxx(uint16_t opcode)
{
    // SKP -> skip

}

static void opcode_Fxxx(uint16_t opcode)
{
    // LD DT

}
