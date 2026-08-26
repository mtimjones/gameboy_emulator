// ppu.c
#include <stdint.h>
#include "cpu.h"
#include "memory.h"
#include "ppu.h"

typedef struct
{
    uint8_t lcdc;
    uint8_t stat;
    uint8_t scy;
    uint8_t scx;
    uint8_t ly;
    uint8_t lyc;
    uint8_t bgp;
    uint8_t obp0;
    uint8_t obp1;
    uint8_t wy;
    uint8_t wx;
    uint16_t line_cycles;
    bool_t lcd_enabled;
    bool_t frame_ready;
    uint32_t framebuffer[PPU_FB_WIDTH * PPU_FB_HEIGHT];
} PPU_t;

static PPU_t ppu;

static inline uint32_t shade_to_rgba(uint8_t shade)
{
    switch (shade & 0x03)
    {
        case 0: return 0xFFFFFFFFu;
        case 1: return 0xFFAAAAAAu;
        case 2: return 0xFF555555u;
        default: return 0xFF000000u;
    }
}

static inline uint8_t palette_lookup(uint8_t palette, uint8_t color)
{
    return (palette >> (color * 2)) & 0x03;
}

static inline uint8_t ppu_mode(void)
{
    return ppu.stat & 0x03;
}

static void ppu_set_mode(uint8_t mode)
{
    uint8_t old_mode = ppu_mode();

    if (old_mode == mode)
        return;

    ppu.stat = (ppu.stat & 0xFC) | (mode & 0x03);

    switch (mode)
    {
        case 0:
            if (ppu.stat & 0x08)
                interrupt_request(0x02);
            break;
        case 1:
            if (ppu.stat & 0x10)
                interrupt_request(0x02);
            break;
        case 2:
            if (ppu.stat & 0x20)
                interrupt_request(0x02);
            break;
    }
}

static void ppu_update_lyc(void)
{
    bool_t coincidence = (ppu.ly == ppu.lyc);
    bool_t old = (ppu.stat & 0x04) != 0;

    if (coincidence)
        ppu.stat |= 0x04;
    else
        ppu.stat &= (uint8_t)~0x04;

    if (coincidence && !old && (ppu.stat & 0x40))
        interrupt_request(0x02);
}

static void ppu_render_scanline(void)
{
    if (ppu.ly >= PPU_FB_HEIGHT)
        return;

    const bool_t bg_enable = (ppu.lcdc & 0x01) != 0;
    const bool_t obj_enable = (ppu.lcdc & 0x02) != 0;
    const bool_t obj_size_16 = (ppu.lcdc & 0x04) != 0;
    const bool_t window_enable = (ppu.lcdc & 0x20) != 0;
    const bool_t window_map = (ppu.lcdc & 0x40) != 0;
    const bool_t bg_map = (ppu.lcdc & 0x08) != 0;
    const bool_t tile_data = (ppu.lcdc & 0x10) != 0;
    const uint16_t bg_tile_map_base = bg_map ? 0x9C00 : 0x9800;
    const uint16_t win_tile_map_base = window_map ? 0x9C00 : 0x9800;

    uint8_t bg_priority[PPU_FB_WIDTH];

    for (unsigned int x = 0; x < PPU_FB_WIDTH; x++)
    {
        uint8_t color = 0;

        if (bg_enable)
        {
            bool_t use_window = window_enable && ppu.ly >= ppu.wy && x + 7 >= ppu.wx;
            uint8_t tile_x;
            uint8_t tile_y;
            uint16_t tile_map_base;

            if (use_window)
            {
                tile_x = (uint8_t)((x + 7) - ppu.wx);
                tile_y = (uint8_t)(ppu.ly - ppu.wy);
                tile_map_base = win_tile_map_base;
            }
            else
            {
                tile_x = (uint8_t)(x + ppu.scx);
                tile_y = (uint8_t)(ppu.ly + ppu.scy);
                tile_map_base = bg_tile_map_base;
            }

            uint8_t map_x = tile_x / 8;
            uint8_t map_y = tile_y / 8;
            uint8_t tile_index = memory_peek8((unsigned short)(tile_map_base + (map_y * 32) + map_x));
            uint16_t tile_addr;

            if (tile_data)
                tile_addr = (uint16_t)(0x8000 + (tile_index * 16));
            else
                tile_addr = (uint16_t)(0x9000 + ((int8_t)tile_index * 16));

            uint8_t row = tile_y & 0x07;
            uint8_t lo = memory_peek8((unsigned short)(tile_addr + row * 2));
            uint8_t hi = memory_peek8((unsigned short)(tile_addr + row * 2 + 1));
            uint8_t bit = (uint8_t)(7 - (tile_x & 0x07));
            color = (uint8_t)(((hi >> bit) & 1u) << 1 | ((lo >> bit) & 1u));
        }

        bg_priority[x] = color;
        ppu.framebuffer[ppu.ly * PPU_FB_WIDTH + x] = shade_to_rgba(palette_lookup(ppu.bgp, color));
    }

    if (!obj_enable)
        return;

    const uint8_t sprite_height = obj_size_16 ? 16 : 8;

    for (unsigned int i = 0; i < 40; i++)
    {
        unsigned int base = i * 4;
        int sprite_y = (int)memory_peek8((unsigned short)(0xFE00 + base)) - 16;
        int sprite_x = (int)memory_peek8((unsigned short)(0xFE00 + base + 1)) - 8;
        uint8_t tile = memory_peek8((unsigned short)(0xFE00 + base + 2));
        uint8_t attr = memory_peek8((unsigned short)(0xFE00 + base + 3));

        if (ppu.ly < sprite_y || ppu.ly >= sprite_y + sprite_height)
            continue;

        int row = ppu.ly - sprite_y;
        if (attr & 0x40)
            row = sprite_height - 1 - row;

        if (obj_size_16)
            tile &= 0xFE;

        uint16_t tile_addr = (uint16_t)(0x8000 + (tile * 16) + (row / 8) * 16);
        uint8_t tile_row = (uint8_t)(row & 0x07);
        uint8_t lo = memory_peek8((unsigned short)(tile_addr + tile_row * 2));
        uint8_t hi = memory_peek8((unsigned short)(tile_addr + tile_row * 2 + 1));
        uint8_t palette = (attr & 0x10) ? ppu.obp1 : ppu.obp0;

        for (unsigned int px = 0; px < 8; px++)
        {
            int x = sprite_x + (attr & 0x20 ? (7 - (int)px) : (int)px);
            if (x < 0 || x >= PPU_FB_WIDTH)
                continue;

            uint8_t bit = (uint8_t)(7 - px);
            uint8_t color = (uint8_t)(((hi >> bit) & 1u) << 1 | ((lo >> bit) & 1u));
            if (color == 0)
                continue;

            if (attr & 0x80)
            {
                if (bg_priority[x] != 0)
                    continue;
            }

            ppu.framebuffer[ppu.ly * PPU_FB_WIDTH + x] = shade_to_rgba(palette_lookup(palette, color));
        }
    }
}

static void ppu_advance_line(void)
{
    if (ppu.ly < 144)
    {
        ppu_render_scanline();
    }

    ppu.ly++;
    ppu_update_lyc();

    if (ppu.ly == 144)
    {
        ppu_set_mode(1);
        interrupt_request(0x01);
        ppu.frame_ready = 1;
    }
    else if (ppu.ly > 153)
    {
        ppu.ly = 0;
        ppu_set_mode(2);
    }
    else if (ppu.ly < 144)
    {
        ppu_set_mode(2);
    }

    ppu.line_cycles = 0;
}

void ppu_init(void)
{
    ppu.lcdc = 0;
    ppu.stat = 0;
    ppu.scy = 0;
    ppu.scx = 0;
    ppu.ly = 0;
    ppu.lyc = 0;
    ppu.bgp = 0xFC;
    ppu.obp0 = 0xFF;
    ppu.obp1 = 0xFF;
    ppu.wy = 0;
    ppu.wx = 0;
    ppu.line_cycles = 0;
    ppu.lcd_enabled = 0;
    ppu.frame_ready = 0;
    for (unsigned int i = 0; i < PPU_FB_WIDTH * PPU_FB_HEIGHT; i++)
        ppu.framebuffer[i] = 0xFFFFFFFFu;
}

bool_t ppu_oam_locked(void)
{
    return ppu.lcd_enabled && (ppu_mode() == 2 || ppu_mode() == 3);
}

bool_t ppu_vram_locked(void)
{
    return ppu.lcd_enabled && ppu_mode() == 3;
}

const uint32_t *ppu_framebuffer(void)
{
    return ppu.framebuffer;
}

bool_t ppu_frame_ready(void)
{
    return ppu.frame_ready;
}

void ppu_clear_frame_ready(void)
{
    ppu.frame_ready = 0;
}

unsigned char ppu_read_reg(unsigned short address)
{
    switch (address)
    {
        case 0xFF40: return ppu.lcdc;
        case 0xFF41: return (ppu.stat & 0xFC) | (ppu_mode() & 0x03) | ((ppu.ly == ppu.lyc) ? 0x04 : 0x00);
        case 0xFF42: return ppu.scy;
        case 0xFF43: return ppu.scx;
        case 0xFF44: return ppu.ly;
        case 0xFF45: return ppu.lyc;
        case 0xFF47: return ppu.bgp;
        case 0xFF48: return ppu.obp0;
        case 0xFF49: return ppu.obp1;
        case 0xFF4A: return ppu.wy;
        case 0xFF4B: return ppu.wx;
        default: return 0xFF;
    }
}

void ppu_write_reg(unsigned short address, unsigned char value)
{
    switch (address)
    {
        case 0xFF40:
        {
            bool_t was_enabled = ppu.lcd_enabled;
            ppu.lcdc = value;
            ppu.lcd_enabled = (value & 0x80) != 0;

            if (!ppu.lcd_enabled)
            {
                ppu.ly = 0;
                ppu.line_cycles = 0;
                ppu.stat = (ppu.stat & 0xFC);
                ppu_update_lyc();
            }
            else if (!was_enabled)
            {
                ppu.ly = 0;
                ppu.line_cycles = 0;
                ppu_set_mode(2);
                ppu_update_lyc();
            }
            break;
        }
        case 0xFF41:
            ppu.stat = (ppu.stat & 0x07) | (value & 0x78);
            break;
        case 0xFF42:
            ppu.scy = value;
            break;
        case 0xFF43:
            ppu.scx = value;
            break;
        case 0xFF44:
            break;
        case 0xFF45:
            ppu.lyc = value;
            ppu_update_lyc();
            break;
        case 0xFF47:
            ppu.bgp = value;
            break;
        case 0xFF48:
            ppu.obp0 = value;
            break;
        case 0xFF49:
            ppu.obp1 = value;
            break;
        case 0xFF4A:
            ppu.wy = value;
            break;
        case 0xFF4B:
            ppu.wx = value;
            break;
    }
}

void ppu_tick(unsigned int cycles)
{
    if (!ppu.lcd_enabled)
        return;

    for (unsigned int i = 0; i < cycles; i++)
    {
        ppu.line_cycles++;

        if (ppu.ly < 144)
        {
            if (ppu.line_cycles == 1)
                ppu_set_mode(2);
            else if (ppu.line_cycles == 80)
                ppu_set_mode(3);
            else if (ppu.line_cycles == 252)
                ppu_set_mode(0);
        }

        if (ppu.line_cycles >= 456)
        {
            ppu_advance_line();
        }
    }
}
