#include "src/emulator.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("No file specified\n");
    }
    else
    {
        emu_load_file(argv[1]);
        emulate();
    }

    return 0;
}
