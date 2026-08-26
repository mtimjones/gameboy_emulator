// memory.c
#include <stdio.h>
#include <stdint.h>
#include "common.h"
#include "assert.h"

extern void gb_tick(unsigned int);

#define ROM_bank_0         0x0000
#define ROM_bank_s         0x4000
#define Video_RAM          0x8000
#define RAM_bank_s         0xA000
#define RAM_bank_i         0xC000
#define RAM_bank_i_echo    0xE000
#define Sprite_Att_RAM     0xFE00
#define Unusable_1         0xFEA0
#define IO_Ports           0xFF00
#define Unusable_2         0xFF4C
#define Internal_RAM       0xFF80
#define INT_EN_REG         0xFFFF

static unsigned char memory[0x10000];
static unsigned char int_en_reg;
static unsigned char int_flag_reg;
static bool_t rom_boot_enabled = true;

typedef struct
{
    uint16_t div_counter;
    uint16_t tima_counter;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
    uint8_t reload_delay;
} Timer_t;

static Timer_t timer;

// dmg_boot.bin
static unsigned char internal_boot_rom[256] = {
/* 0000 */  0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb, 0x7c, 0x20, 0xfb, 0x21, 0x26, 0xff, 0x0e,
/* 0010 */  0x11, 0x3e, 0x80, 0x32, 0xe2, 0x0c, 0x3e, 0xf3, 0xe2, 0x32, 0x3e, 0x77, 0x77, 0x3e, 0xfc, 0xe0,
/* 0020 */  0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd, 0x96, 0x00, 0x13, 0x7b,
/* 0030 */  0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8, 0x00, 0x06, 0x08, 0x1a, 0x13, 0x22, 0x23, 0x05, 0x20, 0xf9,
/* 0040 */  0x3e, 0x19, 0xea, 0x10, 0x99, 0x21, 0x2f, 0x99, 0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20,
/* 0050 */  0xf9, 0x2e, 0x0f, 0x18, 0xf3, 0x67, 0x3e, 0x64, 0x57, 0xe0, 0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04,
/* 0060 */  0x1e, 0x02, 0x0e, 0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20, 0xf7, 0x1d, 0x20, 0xf2,
/* 0070 */  0x0e, 0x13, 0x24, 0x7c, 0x1e, 0x83, 0xfe, 0x62, 0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64, 0x20, 0x06,
/* 0080 */  0x7b, 0xe2, 0x0c, 0x3e, 0x87, 0xe2, 0xf0, 0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20,
/* 0090 */  0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5, 0xcb, 0x11, 0x17, 0xc1, 0xcb, 0x11, 0x17,
/* 00a0 */  0x05, 0x20, 0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9, 0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b,
/* 00b0 */  0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
/* 00c0 */  0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc,
/* 00d0 */  0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e, 0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c,
/* 00e0 */  0x21, 0x04, 0x01, 0x11, 0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20, 0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20,
/* 00f0 */  0xf5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe, 0x3e, 0x01, 0xe0, 0x50
};

void memory_init(void)
{
    rom_boot_enabled = 1;
    int_en_reg = 0;
    int_flag_reg = 0;
    timer.div_counter = 0;
    timer.tima_counter = 0;
    timer.tima = 0;
    timer.tma = 0;
    timer.tac = 0;
    timer.reload_delay = 0;
}

unsigned char interrupt_flags_read(void)
{
    return int_flag_reg;
}

void interrupt_flags_write(unsigned char value)
{
    int_flag_reg = value | 0xE0;
}

unsigned char interrupt_enable_read(void)
{
    return int_en_reg;
}

void interrupt_enable_write(unsigned char value)
{
    int_en_reg = value;
}

void interrupt_request(unsigned char mask)
{
    int_flag_reg |= (mask & 0x1F);
}

static uint16_t timer_period(void)
{
    switch (timer.tac & 0x03)
    {
        case 0: return 1024;
        case 1: return 16;
        case 2: return 64;
        case 3: return 256;
        default: return 1024;
    }
}

void timer_tick(unsigned int cycles)
{
    for (unsigned int i = 0; i < cycles; i++)
    {
        timer.div_counter++;

        if (timer.reload_delay)
        {
            timer.reload_delay--;
            if (timer.reload_delay == 0)
            {
                timer.tima = timer.tma;
                interrupt_request(0x04);
            }
            continue;
        }

        if ((timer.tac & 0x04) == 0)
            continue;

        timer.tima_counter++;
        if (timer.tima_counter >= timer_period())
        {
            timer.tima_counter = 0;
            if (timer.tima == 0xFF)
            {
                timer.tima = 0x00;
                timer.reload_delay = 4;
            }
            else
            {
                timer.tima++;
            }
        }
    }
}

void handle_io_port(unsigned short address, unsigned char value)
{
    switch(address)
    {
        case 0xFF04:
            timer.div_counter = 0;
            memory[address] = 0;
            break;
        case 0xFF05:
            timer.tima = value;
            memory[address] = value;
            break;
        case 0xFF06:
            timer.tma = value;
            memory[address] = value;
            break;
        case 0xFF07:
            timer.tac = value & 0x07;
            timer.tima_counter %= timer_period();
            memory[address] = timer.tac | 0xF8;
            break;
        case 0xFF0F:
            interrupt_flags_write(value);
            memory[address] = int_flag_reg;
            break;
        // Boot ROM Mapping Control
        case 0xFF50:
            if (value != 0) {
                memory[address] = value;
                rom_boot_enabled = 0;
            }
            break;
    }
}

// Hardware address space access.
unsigned char bus_read8(unsigned short address)
{
    if (address < 0x4000) {
        // ROM bank 0
        if (rom_boot_enabled && (address < 256))
        {
            return internal_boot_rom[address];
        }
        return memory[address];
    } else if (address < 0x8000) {
        // Switchable ROM bank
        return memory[address];
    } else if (address < 0xA000) {
        // VRAM
        return memory[address];
    } else if (address < 0xC000) {
        // Cartridge RAM
        return memory[address];
    } else if (address < 0xE000) {
        // Work RAM
        return memory[address];
    } else if (address < 0xFE00) {
        // Echo of work RAM
        return memory[address - 0x2000];
    } else if (address < 0xFEA0) {
        // Sprite attribute table (OAM)
        return memory[address];
    } else if (address < 0xFF00) {
        // Unusable area
        return 0xFF;
    } else if (address < 0xFF4C) {
        // IO ports
        switch (address)
        {
            case 0xFF04: return (unsigned char)(timer.div_counter >> 8);
            case 0xFF05: return timer.tima;
            case 0xFF06: return timer.tma;
            case 0xFF07: return timer.tac | 0xF8;
            case 0xFF0F: return int_flag_reg | 0xE0;
            default: return memory[address];
        }
    } else if (address < 0xFF80) {
        // Unusable area
        return 0xFF;
    } else if (address < 0xFFFF) {
        // High RAM
        return memory[address];
    } else if (address == INT_EN_REG) {
        // Interrupt enable register
        return int_en_reg;
    } else {
        assert(0);
    }
    gb_tick(4);
}

// Hardware address space access.
void bus_write8(unsigned short address, unsigned char value)
{
    if (address < 0x8000) {
        // ROM area, ignored without MBC support
    } else if (address < 0xA000) {
        // VRAM
        memory[address] = value;
    } else if (address < 0xC000) {
        // Cartridge RAM
        memory[address] = value;
    } else if (address < 0xE000) {
        // Work RAM
        memory[address] = value;
    } else if (address < 0xFE00) {
        // Echo of work RAM
        memory[address] = value;
        memory[address - 0x2000] = value;
    } else if (address < 0xFEA0) {
        // OAM
        memory[address] = value;
    } else if (address < 0xFF00) {
        // Unusable area
    } else if (address < 0xFF4C) {
        // IO ports
        handle_io_port(address, value);
        if (address == 0xFF04)
        {
            memory[address] = 0;
        }
        else if (address == 0xFF0F)
        {
            memory[address] = int_flag_reg;
        }
    } else if (address < 0xFF80) {
        // Unusable area
    } else if (address < 0xFFFF) {
        // High RAM
        memory[address] = value;
    } else if (address == INT_EN_REG) {
        // Interrupt enable register
        int_en_reg = value;
    } else {
        printf("Address %x value %x\n", address, value);
        assert(0);
    }
    gb_tick(4);
}
