#include "src/emulator.h"

int main(int argc, char **argv)
{
    emu_load_file(argv[1]);
    emulate();

    return 0;
}
