#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "viewer.h"
#include "common.h"

int main(int argc, char **argv)
{
    memory_init();
    input_init();
    ppu_init();
    cpu_init();

    if (argc > 1)
    {
        if (!memory_load_cartridge(argv[1]))
            return 1;
    }

    if (!viewer_init())
        return 1;
    gb_run();
}
