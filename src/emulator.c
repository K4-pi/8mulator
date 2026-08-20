#include "emulator.h"
#include "keys.h"

#include "SDL3/SDL.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include <stdbool.h>
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

bool is_key_pressed(uint16_t key);
int map_sdl_key_to_chip8(SDL_Keycode keycode);
void set_key_state(uint8_t key, bool is_pressed);

void opcode_0xxx(uint16_t opcode);
void opcode_1xxx(uint16_t opcode);
void opcode_2xxx(uint16_t opcode);
void opcode_3xxx(uint16_t opcode);
void opcode_4xxx(uint16_t opcode);
void opcode_5xxx(uint16_t opcode);
void opcode_6xxx(uint16_t opcode);
void opcode_7xxx(uint16_t opcode);
void opcode_8xxx(uint16_t opcode);
void opcode_9xxx(uint16_t opcode);
void opcode_Axxx(uint16_t opcode);
void opcode_Bxxx(uint16_t opcode);
void opcode_Cxxx(uint16_t opcode);
void opcode_Dxxx(uint16_t opcode);
void opcode_Exxx(uint16_t opcode);
void opcode_Fxxx(uint16_t opcode);

OpcodeHandler opcodes_array[16] = {
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

struct keys_t keys = { 0 };

SDL_Renderer* renderer;
SDL_Event event;

int map_sdl_key_to_chip8(SDL_Keycode keycode)
{
    switch (keycode)
    {
        // Common CHIP-8 keyboard layout:
        // 1 2 3 4
        // Q W E R
        // A S D F
        // Z X C V
        case SDLK_1: return 0x1;
        case SDLK_2: return 0x2;
        case SDLK_3: return 0x3;
        case SDLK_4: return 0xC;
        case SDLK_Q: return 0x4;
        case SDLK_W: return 0x5;
        case SDLK_E: return 0x6;
        case SDLK_R: return 0xD;
        case SDLK_A: return 0x7;
        case SDLK_S: return 0x8;
        case SDLK_D: return 0x9;
        case SDLK_F: return 0xE;
        case SDLK_Z: return 0xA;
        case SDLK_X: return 0x0;
        case SDLK_C: return 0xB;
        case SDLK_V: return 0xF;
        default: return -1;
    }
}

void set_key_state(uint8_t key, bool is_pressed)
{
    switch (key)
    {
        case 0x0: keys.key_0 = is_pressed; break;
        case 0x1: keys.key_1 = is_pressed; break;
        case 0x2: keys.key_2 = is_pressed; break;
        case 0x3: keys.key_3 = is_pressed; break;
        case 0x4: keys.key_4 = is_pressed; break;
        case 0x5: keys.key_5 = is_pressed; break;
        case 0x6: keys.key_6 = is_pressed; break;
        case 0x7: keys.key_7 = is_pressed; break;
        case 0x8: keys.key_8 = is_pressed; break;
        case 0x9: keys.key_9 = is_pressed; break;
        case 0xA: keys.key_A = is_pressed; break;
        case 0xB: keys.key_B = is_pressed; break;
        case 0xC: keys.key_C = is_pressed; break;
        case 0xD: keys.key_D = is_pressed; break;
        case 0xE: keys.key_E = is_pressed; break;
        case 0xF: keys.key_F = is_pressed; break;
        default: break;
    }
}

bool is_key_pressed(uint16_t key)
{
    switch (key)
    {
        case 0x0: return keys.key_0;
        case 0x1: return keys.key_1;
        case 0x2: return keys.key_2;
        case 0x3: return keys.key_3;
        case 0x4: return keys.key_4;
        case 0x5: return keys.key_5;
        case 0x6: return keys.key_6;
        case 0x7: return keys.key_7;
        case 0x8: return keys.key_8;
        case 0x9: return keys.key_9;
        case 0xA: return keys.key_A;
        case 0xB: return keys.key_B;
        case 0xC: return keys.key_C;
        case 0xD: return keys.key_D;
        case 0xE: return keys.key_E;
        case 0xF: return keys.key_F;
        default: return false;
    }
}

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

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background

    while (is_running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                is_running = false;
            }

            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                int chip8_key = map_sdl_key_to_chip8(event.key.key);
                if (chip8_key != -1)
                {
                    set_key_state((uint8_t)chip8_key, true);
                }
            }
            if (event.type == SDL_EVENT_KEY_UP)
            {
                int chip8_key = map_sdl_key_to_chip8(event.key.key);
                if (chip8_key != -1)
                {
                    set_key_state((uint8_t)chip8_key, false);
                }
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
        if (DelayTimer > 0) DelayTimer--;
        if (SoundTimer > 0) SoundTimer--;

        SDL_Delay(20);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void opcode_0xxx(uint16_t opcode)
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
            program_counter = GET_NNN(opcode);
            exit(EXIT_FAILURE);
    }

    program_counter += 2;
}

void opcode_1xxx(uint16_t opcode) // JUMP
{
    program_counter = GET_NNN(opcode);
}

void opcode_2xxx(uint16_t opcode) // CALL
{
    PUSH_STACK(stack_pointer, program_counter);
    program_counter = GET_NNN(opcode);
}

void opcode_3xxx(uint16_t opcode) // SE Vx == kk
{
    uint8_t Vx = GET_VX(opcode);

    if (V[Vx] == GET_KK(opcode)) program_counter += 4;
    else program_counter += 2;
}

void opcode_4xxx(uint16_t opcode) // SNE Vx != kk
{
    uint8_t Vx = GET_VX(opcode);

    if (V[Vx] != GET_KK(opcode)) program_counter += 4;
    else program_counter += 2;
}

void opcode_5xxx(uint16_t opcode) // SE Vx == Vy
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
        printf("opcode 5xxx: %04x not supported\n", opcode);
        exit(EXIT_FAILURE);
    }
}

void opcode_6xxx(uint16_t opcode) // LD Vx = kk
{
    uint8_t Vx = GET_VX(opcode);

    V[Vx] = GET_KK(opcode);

    program_counter += 2;
}

void opcode_7xxx(uint16_t opcode) // ADD Vx = Vx + kk
{
    uint8_t Vx = GET_VX(opcode);

    V[Vx] += GET_KK(opcode);

    program_counter += 2;
}

void opcode_8xxx(uint16_t opcode)
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
            printf("opcode 8xxx: %04X not supported\n", opcode);
            exit(EXIT_FAILURE);
    }

    program_counter += 2;
}

void opcode_9xxx(uint16_t opcode) // SNE Vx, Vy
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
        printf("opcode 9xxx: %04X not supported\n", opcode);
        exit(EXIT_FAILURE);
    }
}

void opcode_Axxx(uint16_t opcode) // I = nnn
{
    I = GET_NNN(opcode);

    program_counter += 2;
}

void opcode_Bxxx(uint16_t opcode) // JP V0, addr
{
    program_counter = V[0] + GET_NNN(opcode);
}

void opcode_Cxxx(uint16_t opcode) // Vx = random byte AND kk
{
    srand(time(0));

    uint8_t Vx = GET_VX(opcode);
    V[Vx] = (rand() % 255 + 1) & GET_KK(opcode);

    program_counter += 2;
}

void opcode_Dxxx(uint16_t opcode) // DRW Vx, Vy, nibble
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

void opcode_Exxx(uint16_t opcode)
{
    switch (opcode & 0xF0FF)
    {
        case 0xE09E:
            if (is_key_pressed(V[GET_VX(opcode)])) program_counter += 2;
            break;

        case 0xE0A1:
            if (!is_key_pressed(V[GET_VX(opcode)])) program_counter += 2;
            break;

        default:
            printf("opcode Exxx: %04X not supported\n", opcode);
            exit(EXIT_FAILURE);
    }

    program_counter += 2;
}

void opcode_Fxxx(uint16_t opcode)
{
    uint8_t Vx = GET_VX(opcode);

    uint16_t offset = I;

    switch (opcode & 0xF0FF)
    {
        case 0xF007:
            V[Vx] = DelayTimer;
            break;

        case 0xF00A:
            bool should_pause = true;

            while (should_pause)
            {
                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_EVENT_KEY_DOWN)
                    {
                        int chip8_key = map_sdl_key_to_chip8(event.key.key);
                        if (chip8_key != -1)
                        {
                            V[Vx] = (uint8_t)chip8_key;
                            should_pause = false;
                        }
                    }
                }
            }
            break;

        case 0xF015:
            DelayTimer = V[Vx];
            break;

        case 0xF018:
            SoundTimer = V[Vx];
            break;

        case 0xF01E:
            I += V[Vx];
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
            printf("opcode Fxxx: %04X not supported\n", opcode);
            break;
    }

    program_counter += 2;
}
