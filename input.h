// input.h

#include <stdint.h>
#include "common.h"

void input_init(void);
unsigned char input_read_ff00(void);
void input_write_ff00(unsigned char value);
void input_key_event(unsigned int vk, bool_t down);
