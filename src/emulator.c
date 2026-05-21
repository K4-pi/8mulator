#include "emulator.h"

#include "SDL3/SDL.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#include <unistd.h>

#define GET_N(x)   (x & 0xF)
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

uint16_t I = 0;
uint8_t V[16] = { 0 };  // Registers in 8-chip architecture

uint8_t DelayTimer = 0;
uint8_t SoundTimer = 0;

uint8_t memory[MEMORY_SIZE] = { 0 };

uint16_t stack[64];
uint16_t *stack_pointer = stack;

uint8_t screen_buffer[64 * 32] = { 0 };

uint16_t program_counter = 0x200;  // 512 == 0x200

SDL_Renderer* renderer;

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
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to init SDL3: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_Window *window = SDL_CreateWindow("8mulator", 640, 320, 0);
    if (!window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Window Creation Failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    bool is_running = true;
    SDL_Event event;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background

    while (is_running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                is_running = false;
            }
        }

        uint8_t left_byte = memory[program_counter];
        uint8_t right_byte = memory[program_counter + 1];

        uint16_t opcode = right_byte | (left_byte << 8);  // shift by 1 byte

        printf("opcode = %04X\n", opcode);

        opcodes_array[(opcode & 0xF000) >> 12](opcode);

        SDL_RenderPresent(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White pixels

        int scale = 5;

        for (int i = 0; i < 64 * 32; i++)
        {
            if (screen_buffer[i])
            {
                int x = (i % 64) * scale;  // wrap around screen
                int y = (i / 64) * scale;

                SDL_FRect rect = {(float)x, (float)y, (float)scale, (float)scale};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
        SDL_Delay(50);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void opcode_0xxx(uint16_t opcode)
{
    switch (opcode)
    {
        case 0x00E0:
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
            SDL_RenderClear(renderer);
            for (int i = 0; i < 64 * 32; i++)
            {
                screen_buffer[i] = 0;
            }
            break;  // CLS

        case 0x00EE:  // RET
            program_counter = POP_STACK(stack_pointer);
            break;

        default:
            printf("opcode: %04X not supported\n", opcode);
            exit(EXIT_FAILURE);
    }

    program_counter += 2;
}

static void opcode_1xxx(uint16_t opcode) // JUMP
{
    program_counter = GET_NNN(opcode) + 2;
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

    program_counter += 2;
}

static void opcode_8xxx(uint16_t opcode)
{
    uint8_t Vx = GET_VX(opcode);
    uint8_t Vy = GET_VY(opcode);

    switch (opcode & 0xF00F)
    {
        case 0x8000:  // Vx = Vy
            V[Vx] = V[Vy];
            break;

        case 0x8001:  // Vx = Vx OR Vy
            V[Vx] |= V[Vy];
            break;

        case 0x8002:  // Vx = Vx AND Vy
            V[Vx] &= V[Vy];
            break;

        case 0x8003:  // Vx = Vx XOR Vy
            V[Vx] ^= V[Vy];
            break;

        case 0x8004:  // Vx = Vx + Vy, set VF = carry
            if ((V[Vx] += V[Vy]) > 0xFF) V[0xF] = 1;
            else V[0xF] = 0;
            break;

        case 0x8005:  // Vx = Vx - Vy, set VF = NOT borrow.
            if (V[Vx] > V[Vy]) V[0xF] = 1;
            else V[0xF] = 0;
            V[Vx] -= V[Vy];
            break;

        case 0x8006:  // Vx = Vx SHR 1.
            if (V[Vx] & 0x1) V[0xF] = 1;
            else V[0xF] = 0;
            V[Vx] /= 2;
            break;

        case 0x8007:  // Vx = Vy - Vx, set VF = NOT borrow
            if (V[Vy] > V[Vx]) V[0xF] = 1;
            else V[0xF] = 0;
            V[Vx] = V[Vy] - V[Vx];
            break;

        case 0x800E:  // Vx = Vx SHL 1
            if (V[Vx] & 0x10) V[0xF] = 1;
            else V[0xF] = 0;
            V[Vx] *= 2;
            break;

        default:
            printf("opcode: %04X not supported\n", opcode);
            exit(EXIT_FAILURE);
    }

    program_counter += 2;
}

static void opcode_9xxx(uint16_t opcode) // SNE Vx, Vy
{
    if ((opcode & 0xF00F) == 0x9000)
    {
        uint8_t Vx = GET_VX(opcode);
        uint8_t Vy = GET_VY(opcode);

        if (V[Vx] != V[Vy]) program_counter += 4;
        else program_counter += 2;
    }
    else
    {
        printf("opcode: %04X not supported\n", opcode);
        exit(EXIT_FAILURE);
    }
}

static void opcode_Axxx(uint16_t opcode) // I = nnn
{
    I = GET_NNN(opcode);

    program_counter += 2;
}

static void opcode_Bxxx(uint16_t opcode) // JP V0, addr
{
    program_counter = V[0] + GET_NNN(opcode);
}

static void opcode_Cxxx(uint16_t opcode) // Vx = random byte AND kk
{
    srand(time(0));

    uint8_t Vx = GET_VX(opcode);
    V[Vx] = (rand() % 255 + 1) & GET_KK(opcode);

    program_counter += 2;
}

static void opcode_Dxxx(uint16_t opcode) // DRW Vx, Vy, nibble
{
    uint8_t Vx = V[GET_VX(opcode)];
    uint8_t Vy = V[GET_VY(opcode)];
    uint8_t len = GET_N(opcode);

    V[0xF] = 0; // Reset collision flag

    for (int row = 0; row < len; row++)
    {
        uint8_t sprite_byte = memory[I + row];

        for (int col = 0; col < 8; col++)  // check bytes in sprite
        {
            if ((sprite_byte & (0x80 >> col)))
            {
                int x = (Vx + col) % 64;
                int y = (Vy + row) % 32;
                int index = y * 64 + x;

                if (screen_buffer[index])  // Check collision if pixel already 1
                {
                    V[0xF] = 1;
                }

                screen_buffer[index] ^= 1;
            }
        }
    }
    program_counter += 2;
}

static void opcode_Exxx(uint16_t opcode)
{
    switch (opcode & 0xF0FF)
    {
        case 0xE09E:
            break;

        case 0xE0A1:
            break;

        default:
            printf("opcode: %04X not supported\n", opcode);
            exit(EXIT_FAILURE);
    }

    program_counter += 2;
}

static void opcode_Fxxx(uint16_t opcode)
{
    uint8_t Vx = V[GET_VX(opcode)];

    uint16_t offset = I;

    switch (opcode & 0xF0FF)
    {
        case 0xF007:
            memory[Vx] = DelayTimer;
            break;

        case 0xF00A:
            break;

        case 0xF015:
            DelayTimer = memory[Vx];
            break;

        case 0xF018:
            SoundTimer = memory[Vx];
            break;

        case 0xF01E:
            I += memory[Vx];
            break;

        case 0xF029:
            break;

        case 0xF033:
            uint16_t ones     = Vx % 10;                    // 243 % 10 = 3
            uint16_t tens     = (Vx % 100) - ones;          // 243 % 100 = 43 - ones = 40
            uint16_t hundreds = (Vx % 1000) - ones - tens;  // 243 % 1000 = 243 - (ones + tens) = 200

            memory[I]   = ones;
            memory[I+1] = tens;
            memory[I+2] = hundreds;
            break;

        case 0xF055:
            for (size_t index = 0; index <= Vx; index++)
            {
                memory[offset] = V[index];
                offset++;
            }
            break;

        case 0xF065:
            for (size_t index = 0; index <= Vx; index++)
            {
                V[index] = memory[offset];
                offset++;
            }
            break;

        default:
            break;
    }

    program_counter += 2;
}
