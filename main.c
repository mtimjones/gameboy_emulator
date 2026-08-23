#include "cpu.h"
#include "memory.h"
#include "common.h"

int main(void)
{
    memory_init();
    cpu_init();
    gb_run();
}
