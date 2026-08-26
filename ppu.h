// ppu.h

#include <stdint.h>
#include "common.h"

#define PPU_FB_WIDTH 160
#define PPU_FB_HEIGHT 144

void ppu_init(void);
void ppu_tick(unsigned int cycles);
unsigned char ppu_read_reg(unsigned short address);
void ppu_write_reg(unsigned short address, unsigned char value);
bool_t ppu_oam_locked(void);
bool_t ppu_vram_locked(void);
const uint32_t *ppu_framebuffer(void);
bool_t ppu_frame_ready(void);
void ppu_clear_frame_ready(void);
