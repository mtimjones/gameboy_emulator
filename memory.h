// memory.h
#include "common.h"
void memory_init();
unsigned char bus_read8(unsigned short address);
void bus_write8(unsigned short address, unsigned char value);
unsigned char memory_peek8(unsigned short address);
bool_t memory_load_cartridge(const char *path);
void timer_tick(unsigned int cycles);
void dma_tick(unsigned int cycles);
unsigned char interrupt_flags_read(void);
void interrupt_flags_write(unsigned char value);
unsigned char interrupt_enable_read(void);
void interrupt_enable_write(unsigned char value);
void interrupt_request(unsigned char mask);
void input_init(void);
