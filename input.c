// input.c
#include <windows.h>
#include <stdint.h>
#include "input.h"
#include "memory.h"

typedef struct
{
    uint8_t select;
    uint8_t keys;
} Input_t;

static Input_t input;

static uint8_t key_mask_for_vk(unsigned int vk)
{
    switch (vk)
    {
        case VK_RIGHT: return 0x01;
        case VK_LEFT:  return 0x02;
        case VK_UP:    return 0x04;
        case VK_DOWN:  return 0x08;
        case 0xBE:     return 0x10; /* '.' */
        case 0xBF:     return 0x20; /* '/' */
        case '1':      return 0x40; /* Select */
        case '2':      return 0x80; /* Start */
        default:       return 0;
    }
}

void input_init(void)
{
    input.select = 0x30;
    input.keys = 0xFF;
}

unsigned char input_read_ff00(void)
{
    unsigned char value = 0xC0 | input.select | 0x0F;

    if ((input.select & 0x10) == 0)
    {
        if ((input.keys & 0x01) == 0) 
	{
            value &= (unsigned char)~0x01;
	}
        if ((input.keys & 0x02) == 0) 
	{
            value &= (unsigned char)~0x02;
	}
        if ((input.keys & 0x04) == 0) 
	{
            value &= (unsigned char)~0x04;
	}
        if ((input.keys & 0x08) == 0) 
	{
            value &= (unsigned char)~0x08;
	}
    }

    if ((input.select & 0x20) == 0)
    {
        if ((input.keys & 0x10) == 0) 
	{
            value &= (unsigned char)~0x01;
	}
        if ((input.keys & 0x20) == 0) 
	{
            value &= (unsigned char)~0x02;
	}
        if ((input.keys & 0x40) == 0) 
	{
            value &= (unsigned char)~0x04;
	}
        if ((input.keys & 0x80) == 0) 
	{
            value &= (unsigned char)~0x08;
	}
    }

    return value;
}

void input_write_ff00(unsigned char value)
{
    input.select = value & 0x30;
}

void input_key_event(unsigned int vk, bool_t down)
{
    uint8_t mask = key_mask_for_vk(vk);

    if (mask == 0)
    {
        return;
    }

    uint8_t previous = input.keys;

    if (down)
    {
        input.keys &= (uint8_t)~mask;
    }
    else
    {
        input.keys |= mask;
    }

    if (down && (previous & mask) != 0)
    {
        interrupt_request(0x10);
    }
}
