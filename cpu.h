// CPU emulator

#include <stdint.h>
#include "common.h"

typedef struct {
    union {
        struct {
            uint8_t F;
            uint8_t A;
        };
        uint16_t AF;
    };

    union {
        struct {
            uint8_t C;
            uint8_t B;
        };
        uint16_t BC;
    };

    union {
        struct {
            uint8_t E;
            uint8_t D;
        };
        uint16_t DE;
    };

    union {
        struct {
            uint8_t L;
            uint8_t H;
        };
        uint16_t HL;
    };

    uint16_t SP;
    uint16_t PC;
} Registers_t;

#define FLAG_Z  0x80  /* Zero */
#define FLAG_N  0x40  /* Subtract */
#define FLAG_H  0x20  /* Half carry */
#define FLAG_C  0x10  /* Carry */

typedef struct
{
    bool_t active;
    uint16_t source;
    uint16_t index;
    uint32_t cycle;
} DMA_t;

typedef struct
{
    Registers_t registers;
    //Timer timer;
    DMA_t dma;

    uint64_t tcycles;
    bool_t running;
    bool_t halted;
    bool_t stopped;
    bool_t ime_delay;  // Delay the interrupt enable.
    bool_t ime_pending;
    bool_t ime;
} CPU_t;

void cpu_init();
void gb_run();