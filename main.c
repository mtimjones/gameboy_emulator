#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "viewer.h"
#include "common.h"

int main(void)
{
    memory_init();
    input_init();
    ppu_init();
    cpu_init();
    if (!viewer_init())
        return 1;
    gb_run();
}
