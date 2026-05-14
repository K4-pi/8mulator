#include "emulator.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MEMORY_SIZE 4096  // 4096 == 0xFFF

uint16_t I;
uint8_t V[16] = { 0 };  // Registers in 8-chip architecture

uint8_t memory[MEMORY_SIZE] = { 0 };
uint8_t stack_pointer = 0;

uint16_t program_counter = 0x200;  // 512 == 0x200

void emu_load_file(const char *filename)
{
    FILE *f;
    if ((f = fopen(filename, "r")) == NULL)
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
}

void emulate()
{
    uint8_t left_byte = memory[program_counter];
    uint8_t right_byte = memory[program_counter + 1];

    printf("left = %02X\n", left_byte);
    printf("right = %02X\n", right_byte);

    uint16_t opcode = right_byte | (left_byte << 8);  // shift by 1 byte

    printf("opcode = %04X\n\n", opcode);

    switch (opcode & 0xF000)
    {
        case 0x8000:
            // ADD, AND, XOR
            program_counter += 2;
            break;

        default:
            printf("opcode: %04X not supported", opcode & 0xF000);
            exit(EXIT_FAILURE);
    }
}
