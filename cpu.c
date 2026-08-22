// CPU emulator
#include "cpu.h"
#include "memory.h"

CPU_t cpu;

void cpu_init(void)
{
    cpu.tcycles = 0;

}

void gb_tick(unsigned int cycles)
{
    for (unsigned int i = 0 ; i < cycles; i++)
    {
        //timer_tick(1);
        //ppu_tick(1); // Picture Processing Unit
        //dma_tick(1); 
        cpu.tcycles++; 
    } 
} 

// CPU operation that uses the bus and consumes CPU cycles. 
uint8_t cpu_read8(uint16_t address) 
{ 
    uint8_t value = bus_read8(address); 
    gb_tick(4); 
    return value; 
} 

// CPU operation that uses the bus and consumes CPU cycles. 
void cpu_write8(uint16_t address, uint8_t value) 
{ 
    bus_write8(address, value); 
    gb_tick(4); 
} 

// CPU operation that uses the bus and consumes CPU cycles. 
uint8_t cpu_fetch8() 
{ 
    uint8_t value = cpu_read8(cpu.registers.PC); 
    cpu.registers.PC++; 
    return value; 
} 

void cpu_idle() 
{ 
    gb_tick(4); 
} 

static void cpu_update_ime() 
{ 
    if (cpu.ime_delay) { 
        cpu.ime_delay--; 

        if (cpu.ime_delay == 0) 
            cpu.ime = 1; 
    } 
} 

void cpu_step() 
{ 
    if (cpu.halted) 
    { 
        gb_tick(4); 
        //cpu_check_interrupts(); 
    } 

    uint8_t opcode = cpu_fetch8(); 
    //opcode_table[opcode](); 
    //cpu_check_interrupts(); 

    cpu_update_ime(); 
} 

void gb_run() 
{ 
    while(cpu.running) 
    { 
        cpu_step(); 
    } 
} 

#if 0 
static inline uint8_t flags_add_sp_e8(uint16_t sp, uint8_t imm) 
{ 
    uint8_t flags = 0; 

    if ((sp & 0x0F) + (imm & 0x0F) > 0x0F) 
    { 
        flags |= FLAG_H; 
    } 

    if ((sp & 0xFF) + imm > 0xFF) 
    { 
        flags |= FLAG_C; 
    } 

    return flags; 
} 
#endif 

static inline uint8_t cpu_add8(uint8_t lhs, uint8_t rhs) 
{ 
    uint16_t result = (uint16_t)lhs + rhs; 

    cpu.registers.F = 0; 

    if ((uint8_t)result == 0) 
        cpu.registers.F |= FLAG_Z; 

    if (((lhs & 0x0F) + (rhs & 0x0F)) > 0x0F) 
        cpu.registers.F |= FLAG_H; 

    if (result > 0xFF) 
        cpu.registers.F |= FLAG_C; 

    return (uint8_t)result; 
} 

static inline uint8_t cpu_adc8(uint8_t lhs, uint8_t rhs) 
{ 
    uint8_t carry = (cpu.registers.F & FLAG_C) ? 1 : 0; 
    uint16_t result = (uint16_t)lhs + rhs + carry; 

    cpu.registers.F = 0; 

    if ((uint8_t)result == 0) 
        cpu.registers.F |= FLAG_Z; 

    if (((lhs & 0x0F) + (rhs & 0x0F) + carry) > 0x0F) 
        cpu.registers.F |= FLAG_H; 

    if (result > 0xFF) 
        cpu.registers.F |= FLAG_C; 

    return (uint8_t)result; 
} 

static inline uint8_t cpu_sub8(uint8_t lhs, uint8_t rhs) 
{ 
    uint8_t result = lhs - rhs; 

    cpu.registers.F = FLAG_N; 

    if (result == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    if ((lhs & 0x0F) < (rhs & 0x0F)) 
    { 
        cpu.registers.F |= FLAG_H; 
    } 

    if (lhs < rhs) 
    { 
        cpu.registers.F |= FLAG_C; 
    } 

    return result; 
} 

static inline uint8_t cpu_sbc8(uint8_t lhs, uint8_t rhs) 
{ 
    uint8_t carry = (cpu.registers.F & FLAG_C) ? 1 : 0; 
    uint8_t result = lhs - rhs - carry; 

    cpu.registers.F = FLAG_N; 

    if (result == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    if ((lhs & 0x0F) < ((rhs & 0x0F) + carry)) 
    { 
        cpu.registers.F |= FLAG_H; 
    } 

    if ((uint16_t)lhs < (uint16_t)rhs + carry) 
    { 
        cpu.registers.F |= FLAG_C; 
    } 

    return result; 
} 

static inline uint8_t cpu_and8(uint8_t value) 
{ 
    cpu.registers.A &= value; 

    cpu.registers.F = FLAG_H; 

    if (cpu.registers.A == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    return cpu.registers.A; 
} 

static inline void cpu_or8(uint8_t value) 
{ 
    cpu.registers.A |= value; 

    cpu.registers.F = 0; 

    if (cpu.registers.A == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 
} 

static inline void cpu_xor8(uint8_t value) 
{ 
    cpu.registers.A ^= value; 

    cpu.registers.F = 0; 

    if (cpu.registers.A == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 
} 

static inline void cpu_cp8(uint8_t value) 
{ 
    uint8_t a = cpu.registers.A; 
    uint8_t result = a - value; 

    cpu.registers.F = FLAG_N; 

    if (result == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    if ((a & 0x0F) < (value & 0x0F)) 
    { 
        cpu.registers.F |= FLAG_H; 
    } 

    if (a < value) 
    { 
        cpu.registers.F |= FLAG_C; 
    } 
} 

static inline uint8_t cpu_inc8(uint8_t value) 
{ 
    uint8_t result = value + 1; 

    /* Preserve carry, clear Z/N/H */ 
    cpu.registers.F &= FLAG_C; 

    if (result == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    if ((value & 0x0F) == 0x0F) 
    { 
        cpu.registers.F |= FLAG_H; 
    } 

    return result; 
} 

static inline uint8_t cpu_dec8(uint8_t value) 
{ 
    uint8_t result = value - 1; 

    /* Preserve carry, clear Z/H, set N */ 
    cpu.registers.F = (cpu.registers.F & FLAG_C) | FLAG_N; 

    if (result == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    if ((value & 0x0F) == 0x00) 
    { 
        cpu.registers.F |= FLAG_H; 
    } 

    return result; 
} 

static inline void cpu_add_hl(uint16_t value) 
{ 
    uint16_t hl = cpu.registers.HL; 
    uint32_t result = (uint32_t)hl + value; 

    /* Preserve Z, clear N/H/C */ 
    cpu.registers.F &= FLAG_Z; 

    if (((hl & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF) 
    { 
        cpu.registers.F |= FLAG_H; 
    } 

    if (result > 0xFFFF) 
    { 
        cpu.registers.F |= FLAG_C; 
    } 

    cpu.registers.HL = (uint16_t)result; 
} 

static inline uint8_t flags_add_sp_e8(uint16_t sp, uint8_t imm) 
{ 
    uint8_t flags = 0; 

    if (((sp & 0x0F) + (imm & 0x0F)) > 0x0F) 
    { 
        flags |= FLAG_H; 
    } 

    if (((sp & 0xFF) + imm) > 0xFF) 
    { 
        flags |= FLAG_C; 
    } 

    return flags; 
} 

static inline uint8_t cpu_swap8(uint8_t value) 
{ 
    uint8_t result = (value << 4) | (value >> 4); 

    cpu.registers.F = 0; 

    if (result == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    return result; 
} 

static inline uint8_t cpu_rlc8(uint8_t value) 
{ 
    uint8_t carry = (value >> 7) & 0x01; 
    uint8_t result = (value << 1) | carry; 

    cpu.registers.F = 0; 

    if (result == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    if (carry) 
    { 
        cpu.registers.F |= FLAG_C; 
    } 

    return result; 
} 

static inline uint8_t cpu_rl8(uint8_t value) 
{ 
    uint8_t old_carry = (cpu.registers.F & FLAG_C) ? 1 : 0; 
    uint8_t new_carry = (value >> 7) & 0x01; 

    uint8_t result = (value << 1) | old_carry; 

    cpu.registers.F = 0; 

    if (result == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    if (new_carry) 
    { 
        cpu.registers.F |= FLAG_C; 
    } 

    return result; 
} 

static inline uint8_t cpu_rrc8(uint8_t value) 
{ 
    uint8_t carry = value & 0x01; 
    uint8_t result = (value >> 1) | (carry << 7); 

    cpu.registers.F = 0; 

    if (result == 0) 
        cpu.registers.F |= FLAG_Z; 

    if (carry) 
        cpu.registers.F |= FLAG_C; 

    return result; 
} 

static void op_00() 
{ 
    // NOP 
} 

// LD BC, $#### 
static void op_01() 
{ 
    uint8_t lo = cpu_fetch8(); 
    uint8_t hi = cpu_fetch8(); 
    cpu.registers.BC = ((uint16_t)hi << 8) | lo; 
} 

static void op_02() 
{ 
    uint16_t address = bus_read8(cpu.registers.BC); 
    bus_write8(address, cpu.registers.A); 
} 

static void op_03() 
{ 
    cpu.registers.BC++; 
    cpu_idle(); 
} 

static void op_04() 
{ 
    cpu.registers.B = cpu_inc8(cpu.registers.B); 
} 

static void op_05() 
{ 
    cpu.registers.B = cpu_dec8(cpu.registers.B); 
} 

static void op_06() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.B = val; 
} 

void op_07() 
{ 
    uint8_t a = cpu.registers.A; 
    uint8_t carry = (a >> 7) & 1; 

    cpu.registers.A = (a << 1) | carry; 

    cpu.registers.F = carry ? FLAG_C : 0; 
} 

static void op_08() 
{ 
    uint8_t lo = cpu_fetch8(); 
    uint8_t hi = cpu_fetch8(); 
    uint16_t address = (hi << 8) | lo; 
    bus_write8(address, cpu.registers.SP); 
} 

static void op_09() 
{ 
    cpu_add_hl(cpu.registers.BC); 
    cpu_idle(); 
} 

static void op_0A() 
{ 
    cpu.registers.A = bus_read8(cpu.registers.BC); 
} 

static void op_0B() 
{ 
    cpu.registers.BC--; 
    cpu_idle(); 
} 

static void op_0C() 
{ 
    cpu.registers.C = cpu_inc8(cpu.registers.C); 
} 

static void op_0D() 
{ 
    cpu.registers.C = cpu_dec8(cpu.registers.C); 
} 

static void op_0e() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.C = val; 
} 

void op_0F() 
{ 
    uint8_t a = cpu.registers.A; 
    uint8_t carry = a & 0x01; 

    cpu.registers.A = (a >> 1) | (carry << 7); 

    cpu.registers.F = carry ? FLAG_C : 0; 
} 

// TBD 
static void op_10() 
{ 
} 

static void op_11() 
{ 
    uint8_t lo = cpu_fetch8(); 
    uint8_t hi = cpu_fetch8(); 
    cpu.registers.DE = ((uint16_t)hi << 8) | lo; 
} 

static void op_12() 
{ 
    uint16_t address = bus_read8(cpu.registers.DE); 
    bus_write8(cpu.registers.DE, cpu.registers.A); 
} 

static void op_13() 
{ 
    cpu.registers.DE++; 
    cpu_idle(); 
} 

static void op_14() 
{ 
    cpu.registers.D = cpu_inc8(cpu.registers.D); 
} 

static void op_15() 
{ 
    cpu.registers.D = cpu_dec8(cpu.registers.D); 
} 

static void op_16() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.D = val; 
} 

void op_17() 
{ 
    uint8_t a = cpu.registers.A; 
    uint8_t old_carry = (cpu.registers.F & FLAG_C) ? 1 : 0; 
    uint8_t new_carry = (a >> 7) & 1; 

    cpu.registers.A = (a << 1) | old_carry; 

    cpu.registers.F = new_carry ? FLAG_C : 0; 
} 

static void op_19() 
{ 
    cpu_add_hl(cpu.registers.DE); 
    cpu_idle(); 
} 

static void op_1A() 
{ 
    cpu.registers.A = bus_read8(cpu.registers.DE); 
} 

static void op_1B() 
{ 
    cpu.registers.DE--; 
    cpu_idle(); 
} 

static void op_1C() 
{ 
    cpu.registers.E = cpu_inc8(cpu.registers.E); 
} 

static void op_1D() 
{ 
    cpu.registers.E = cpu_dec8(cpu.registers.E); 
} 

static void op_1e() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.E = val; 
} 

void op_1F() 
{ 
    uint8_t a = cpu.registers.A; 
    uint8_t old_carry = (cpu.registers.F & FLAG_C) ? 1 : 0; 
    uint8_t new_carry = a & 0x01; 

    cpu.registers.A = (a >> 1) | (old_carry << 7); 

    cpu.registers.F = new_carry ? FLAG_C : 0; 
} 

static void op_21() 
{ 
    uint8_t lo = cpu_fetch8(); 
    uint8_t hi = cpu_fetch8(); 
    cpu.registers.HL = ((uint16_t)hi << 8) | lo; 
} 

static void op_22() 
{ 
    bus_write8(cpu.registers.HL, cpu.registers.A); 
    cpu.registers.HL = cpu.registers.HL + 1; 
} 

static void op_23() 
{ 
    cpu.registers.HL++; 
    cpu_idle(); 
} 

static void op_24() 
{ 
    cpu.registers.H = cpu_inc8(cpu.registers.H); 
} 

static void op_25() 
{ 
    cpu.registers.H = cpu_dec8(cpu.registers.H); 
} 

static void op_26() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.H = val; 
} 

void op_27() 
{ 
    uint8_t a = cpu.registers.A; 
    uint8_t adjust = 0; 
    bool_t carry = (cpu.registers.F & FLAG_C) != 0; 

    if (cpu.registers.F & FLAG_N) { 
        /* After SUB/SBC */ 
        if (cpu.registers.F & FLAG_H) 
            adjust |= 0x06; 

        if (carry) 
            adjust |= 0x60; 

        a -= adjust; 
    } 
    else { 
        /* After ADD/ADC */ 
        if ((cpu.registers.F & FLAG_H) || (a & 0x0F) > 0x09) 
            adjust |= 0x06; 

        if (carry || a > 0x99) { 
            adjust |= 0x60; 
            carry = 1; 
        } 

        a += adjust; 
    } 

    cpu.registers.A = a; 

    /* Preserve N; DAA always clears H. */ 
    cpu.registers.F &= FLAG_N; 

    if (a == 0) 
    { 
        cpu.registers.F |= FLAG_Z; 
    } 

    if (carry) 
    { 
        cpu.registers.F |= FLAG_C; 
    } 
} 

static void op_29() 
{ 
    cpu_add_hl(cpu.registers.HL); 
    cpu_idle(); 
} 

static void op_2A() 
{ 
    cpu.registers.A = bus_read8(cpu.registers.HL); 
    cpu.registers.HL = cpu.registers.HL + 1; 
} 

static void op_2B() 
{ 
    cpu.registers.HL--; 
    cpu_idle(); 
} 

static void op_2C() 
{ 
    cpu.registers.L = cpu_inc8(cpu.registers.L); 
} 

static void op_2D() 
{ 
    cpu.registers.L = cpu_dec8(cpu.registers.L); 
} 

static void op_2e() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.L = val; 
} 

void op_cpl() 
{ 
    cpu.registers.A = ~cpu.registers.A; 
    cpu.registers.F |= FLAG_N | FLAG_H; 
} 

static void op_31() 
{ 
    uint8_t lo = cpu_fetch8(); 
    uint8_t hi = cpu_fetch8(); 
    cpu.registers.SP = ((uint16_t)hi << 8) | lo; 
} 

static void op_32() 
{ 
    bus_write8(cpu.registers.HL, cpu.registers.A); 
    cpu.registers.HL = cpu.registers.HL - 1; 
} 

static void op_33() 
{ 
    cpu.registers.SP++; 
    cpu_idle(); 
} 

static void op_34() 
{ 
    uint8_t value = bus_read8(cpu.registers.HL); 
    value = cpu_inc8(value); 
    bus_write8(cpu.registers.HL, value); 
} 

static void op_35() 
{ 
    uint8_t value = bus_read8(cpu.registers.HL); 
    bus_write8(cpu.registers.HL, value); 
} 

void op_37() 
{ 
    /* Preserve Z, clear N/H, set C */ 
    cpu.registers.F = (cpu.registers.F & FLAG_Z) | FLAG_C; 
} 

static void op_39() 
{ 
    cpu_add_hl(cpu.registers.SP); 
    cpu_idle(); 
} 

static void op_3A() 
{ 
    cpu.registers.A = bus_read8(cpu.registers.HL); 
    cpu.registers.HL = cpu.registers.HL - 1; 
} 

static void op_3B() 
{ 
    cpu.registers.SP--; 
    cpu_idle(); 
} 

static void op_3C() 
{ 
    cpu.registers.A = cpu_inc8(cpu.registers.A); 
} 

static void op_3D() 
{ 
    cpu.registers.A = cpu_dec8(cpu.registers.A); 
} 

static void op_3E() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.A = val; 
} 

void op_3F() 
{ 
    uint8_t carry = cpu.registers.F & FLAG_C; 

    /* Preserve Z, clear N/H/C */ 
    cpu.registers.F &= FLAG_Z; 

    /* Invert carry */ 
    if (!carry) 
    { 
        cpu.registers.F |= FLAG_C; 
    } 
} 

static void op_40() 
{ 
    cpu.registers.B = cpu.registers.B; 
} 

static void op_41() 
{ 
    cpu.registers.B = cpu.registers.C; 
} 

static void op_42() 
{ 
    cpu.registers.B = cpu.registers.D; 
} 

static void op_43() 
{ 
    cpu.registers.B = cpu.registers.E; 
} 

static void op_44() 
{ 
    cpu.registers.B = cpu.registers.H; 
} 

static void op_45() 
{ 
    cpu.registers.B = cpu.registers.L; 
} 

static void op_46() 
{ 
    cpu.registers.B = bus_read8(cpu.registers.HL); 
} 

static void op_47() 
{ 
    cpu.registers.B = cpu.registers.A; 
} 

static void op_48() 
{ 
    cpu.registers.C = cpu.registers.B; 
} 

static void op_49() 
{ 
    cpu.registers.C = cpu.registers.C; 
} 

static void op_4A() 
{ 
    cpu.registers.C = cpu.registers.D; 
} 

static void op_4B() 
{ 
    cpu.registers.C = cpu.registers.E; 
} 

static void op_4C() 
{ 
    cpu.registers.C = cpu.registers.H; 
} 

static void op_4D() 
{ 
    cpu.registers.C = cpu.registers.L; 
} 

static void op_4E() 
{ 
    cpu.registers.C = bus_read8(cpu.registers.HL); 
} 

static void op_50() 
{ 
    cpu.registers.D = cpu.registers.B; 
} 

static void op_51() 
{ 
    cpu.registers.D = cpu.registers.C; 
} 

static void op_52() 
{ 
    cpu.registers.D = cpu.registers.D; 
} 

static void op_53() 
{ 
    cpu.registers.D = cpu.registers.E; 
} 

static void op_54() 
{ 
    cpu.registers.D = cpu.registers.H; 
} 

static void op_55() 
{ 
    cpu.registers.D = cpu.registers.L; 
} 

static void op_56() 
{ 
    cpu.registers.D = bus_read8(cpu.registers.HL); 
} 

static void op_57() 
{ 
    cpu.registers.D = cpu.registers.A; 
} 

static void op_58() 
{ 
    cpu.registers.E = cpu.registers.B; 
} 

static void op_59() 
{ 
    cpu.registers.E = cpu.registers.C; 
} 

static void op_5A() 
{ 
    cpu.registers.E = cpu.registers.D; 
} 

static void op_5B() 
{ 
    cpu.registers.E = cpu.registers.E; 
} 

static void op_5C() 
{ 
    cpu.registers.E = cpu.registers.H; 
} 

static void op_5D() 
{ 
    cpu.registers.E = cpu.registers.L; 
} 

static void op_5E() 
{ 
    cpu.registers.E = bus_read8(cpu.registers.HL); 
} 

static void op_5F() 
{ 
    cpu.registers.E = cpu.registers.A; 
} 

static void op_60() 
{ 
    cpu.registers.H = cpu.registers.L; 
} 

static void op_61() 
{ 
    cpu.registers.H = cpu.registers.C; 
} 

static void op_62() 
{ 
    cpu.registers.H = cpu.registers.D; 
} 

static void op_63() 
{ 
    cpu.registers.H = cpu.registers.E; 
} 

static void op_64() 
{ 
    cpu.registers.H = cpu.registers.H; 
} 

static void op_65() 
{ 
    cpu.registers.H = cpu.registers.L; 
} 

static void op_66() 
{ 
    cpu.registers.H = bus_read8(cpu.registers.HL); 
} 

static void op_67() 
{ 
    cpu.registers.H = cpu.registers.A; 
} 

static void op_68() 
{ 
    cpu.registers.L = cpu.registers.B; 
} 

static void op_69() 
{ 
    cpu.registers.L = cpu.registers.C; 
} 

static void op_6A() 
{ 
    cpu.registers.L = cpu.registers.D; 
} 

static void op_6B() 
{ 
    cpu.registers.L = cpu.registers.E; 
} 

static void op_6C() 
{ 
    cpu.registers.L = cpu.registers.H; 
} 

static void op_6D() 
{ 
    cpu.registers.L = cpu.registers.L; 
} 

static void op_6E() 
{ 
    cpu.registers.L = bus_read8(cpu.registers.HL); 
} 

static void op_6F() 
{ 
    cpu.registers.L = cpu.registers.A; 
} 

static void op_70() 
{ 
    bus_write8(cpu.registers.HL, cpu.registers.B); 
} 

static void op_71() 
{ 
    bus_write8(cpu.registers.HL, cpu.registers.C); 
} 

static void op_72() 
{ 
    bus_write8(cpu.registers.HL, cpu.registers.D); 
} 

static void op_73() 
{ 
    bus_write8(cpu.registers.HL, cpu.registers.E); 
} 

static void op_74() 
{ 
    bus_write8(cpu.registers.HL, cpu.registers.H); 
} 

static void op_75() 
{ 
    bus_write8(cpu.registers.HL, cpu.registers.L); 
} 

static void op_76() 
{ 
    // TBD 
} 

static void op_77() 
{ 
    uint16_t address = bus_read8(cpu.registers.HL); 
    bus_write8(address, cpu.registers.A); 
} 

static void op_78() 
{ 
    cpu.registers.A = cpu.registers.B; 
} 

static void op_79() 
{ 
    cpu.registers.A = cpu.registers.C; 
} 

static void op_7A() 
{ 
    cpu.registers.A = cpu.registers.D; 
} 

static void op_7B() 
{ 
    cpu.registers.A = cpu.registers.E; 
} 

static void op_7C() 
{ 
    cpu.registers.A = cpu.registers.H; 
} 

static void op_7D() 
{ 
    cpu.registers.A = cpu.registers.L; 
} 

static void op_7E() 
{ 
    cpu.registers.A = bus_read8(cpu.registers.HL); 
} 

static void op_7F() 
{ 
    cpu.registers.A = cpu.registers.A; 
} 

static void op_80() 
{ 
    cpu_add8(cpu.registers.A, cpu.registers.B); 
} 

static void op_81() 
{ 
    cpu_add8(cpu.registers.A, cpu.registers.C); 
} 

static void op_82() 
{ 
    cpu_add8(cpu.registers.A, cpu.registers.D); 
} 

static void op_83() 
{ 
    cpu_add8(cpu.registers.A, cpu.registers.E); 
} 

static void op_84() 
{ 
    cpu_add8(cpu.registers.A, cpu.registers.H); 
} 

static void op_85() 
{ 
    cpu_add8(cpu.registers.A, cpu.registers.L); 
} 

static void op_86() 
{ 
    uint8_t val = bus_read8(cpu.registers.HL); 
    cpu_add8(cpu.registers.A, val); 
} 

static void op_87() 
{ 
    cpu_add8(cpu.registers.A, cpu.registers.A); 
} 

static void op_88() 
{ 
    cpu.registers.A = cpu_adc8(cpu.registers.A, cpu.registers.B); 
} 

static void op_89() 
{ 
    cpu.registers.A = cpu_adc8(cpu.registers.A, cpu.registers.C); 
} 

static void op_8A() 
{ 
    cpu.registers.A = cpu_adc8(cpu.registers.A, cpu.registers.D); 
} 

static void op_8B() 
{ 
    cpu.registers.A = cpu_adc8(cpu.registers.A, cpu.registers.E); 
} 

static void op_8C() 
{ 
    cpu.registers.A = cpu_adc8(cpu.registers.A, cpu.registers.H); 
} 

static void op_8D() 
{ 
    cpu.registers.A = cpu_adc8(cpu.registers.A, cpu.registers.L); 
} 

static void op_8E() 
{ 
    uint8_t val = bus_read8(cpu.registers.HL); 
    cpu.registers.A = cpu_adc8(cpu.registers.A, val); 
} 

static void op_8F() 
{ 
    cpu.registers.A = cpu_adc8(cpu.registers.A, cpu.registers.A); 
} 

void op_90() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.B); 
} 

void op_91() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.C); 
} 

void op_92() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.D); 
} 

void op_93() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.E); 
} 

void op_94() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.H); 
} 

void op_95() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.L); 
} 

void op_96() 
{ 
    uint8_t val = bus_read8(cpu.registers.HL); 
    cpu.registers.A = cpu_sub8(cpu.registers.A, val); 
} 

void op_97() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.A); 
} 

void op_98() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.B); 
} 

void op_99() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.C); 
} 

void op_9A() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.D); 
} 

void op_9B() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.E); 
} 

void op_9C() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.H); 
} 

void op_9D() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.L); 
} 

void op_9E() 
{ 
    uint8_t val = bus_read8(cpu.registers.HL); 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, val); 
} 

void op_9F() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.A); 
} 

void op_A0() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.B); 
} 

void op_A1() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.C); 
} 

void op_A2() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.D); 
} 

void op_A3() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.E); 
} 

void op_A4() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.H); 
} 

void op_A5() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.L); 
} 

void op_A6() 
{ 
    uint8_t value = bus_read8(cpu.registers.HL); 
    cpu.registers.A = cpu_and8(value); 
} 

void op_A7() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.A); 
} 

void op_A8() 
{ 
    cpu_xor8(cpu.registers.B); 
} 

void op_A9() 
{ 
    cpu_xor8(cpu.registers.C); 
} 

void op_AA() 
{ 
    cpu_xor8(cpu.registers.D); 
} 

void op_AB() 
{ 
    cpu_xor8(cpu.registers.E); 
} 

void op_AC() 
{ 
    cpu_xor8(cpu.registers.H); 
} 

void op_AD() 
{ 
    cpu_xor8(cpu.registers.L); 
} 

void op_AE() 
{ 
    uint8_t value = bus_read8(cpu.registers.HL); 
    cpu_xor8(value); 
} 

void op_AF() 
{ 
    cpu_xor8(cpu.registers.A); 
} 

void op_B0() 
{ 
    cpu_or8(cpu.registers.B); 
} 

void op_B1() 
{ 
    cpu_or8(cpu.registers.C); 
} 

void op_B2() 
{ 
    cpu_or8(cpu.registers.D); 
} 

void op_B3() 
{ 
    cpu_or8(cpu.registers.E); 
} 

void op_B4() 
{ 
    cpu_or8(cpu.registers.H); 
} 

void op_B5() 
{ 
    cpu_or8(cpu.registers.L); 
} 

void op_B6() 
{ 
    uint8_t value = bus_read8(cpu.registers.HL); 
    cpu_or8(value); 
} 

void op_B7() 
{ 
    cpu_or8(cpu.registers.A); 
} 

static void op_B8() 
{ 
    cpu_cp8(cpu.registers.B); 
} 

static void op_B9() 
{ 
    cpu_cp8(cpu.registers.C); 
} 

static void op_BA() 
{ 
    cpu_cp8(cpu.registers.D); 
} 

static void op_BB() 
{ 
    cpu_cp8(cpu.registers.E); 
} 

static void op_BC() 
{ 
    cpu_cp8(cpu.registers.H); 
} 

static void op_BD() 
{ 
    cpu_cp8(cpu.registers.L); 
} 

static void op_BE() 
{ 
    uint8_t value = bus_read8(cpu.registers.HL); 
    cpu_cp8(value); 
} 

void op_C5() 
{ 
    cpu_idle();  /* internal M-cycle */ 

    cpu.registers.SP--; 
    cpu_write8(cpu.registers.SP, cpu.registers.B); 

    cpu.registers.SP--; 
    cpu_write8(cpu.registers.SP, cpu.registers.C); 
} 

static void op_C6() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu_add8(cpu.registers.A, val); 
} 

static void op_CB() 
{ 
    uint8_t subcode = cpu_fetch8(); 
    uint8_t value, result; 
    switch(subcode) 
    { 
        case 0x0E: 
            uint8_t value = bus_read8(cpu.registers.HL); 
            uint8_t result = cpu_rrc8(value); 
            bus_write8(cpu.registers.HL, result); 
            break;
        case 0x0D:
            cpu.registers.L = cpu_rrc8(cpu.registers.L);
            break;
        case 0x0C:
            cpu.registers.H = cpu_rrc8(cpu.registers.H);
            break;
        case 0x0B:
            cpu.registers.E = cpu_rrc8(cpu.registers.E);
            break;
        case 0x0A:
            cpu.registers.D = cpu_rrc8(cpu.registers.D);
            break;
        case 0x09:
            cpu.registers.C = cpu_rrc8(cpu.registers.C);
            break;
        case 0x08:
            cpu.registers.B = cpu_rrc8(cpu.registers.B);
            break;
        case 0x0f:
            cpu.registers.A = cpu_rrc8(cpu.registers.A);
            break;
        case 0x37:
            cpu.registers.A = cpu_swap8(cpu.registers.A);
            break;
        case 0x30:
            cpu.registers.B = cpu_swap8(cpu.registers.B);
            break;
        case 0x31:
            cpu.registers.C = cpu_swap8(cpu.registers.B);
            break;
        case 0x32:
            cpu.registers.D = cpu_swap8(cpu.registers.D);
            break;
        case 0x33:
            cpu.registers.E = cpu_swap8(cpu.registers.E);
            break;
        case 0x34:
            cpu.registers.H = cpu_swap8(cpu.registers.H);
            break;
        case 0x35:
            cpu.registers.L = cpu_swap8(cpu.registers.L);
            break;
        case 0x36:
            bus_write8(cpu.registers.HL,(cpu_swap8(bus_read8(cpu.registers.HL))));
            break;
        case 0x07:
            // RLC A
            cpu.registers.A = cpu_rlc8(cpu.registers.A);
            break;
        case 0x00:
            // RLC B
            cpu.registers.B = cpu_rlc8(cpu.registers.B);
            break;
        case 0x01:
            // RLC C
            cpu.registers.C = cpu_rlc8(cpu.registers.C);
            break;
        case 0x02:
            // RLC D
            cpu.registers.D = cpu_rlc8(cpu.registers.D);
            break;
        case 0x03:
            // RLC E
            cpu.registers.E = cpu_rlc8(cpu.registers.E);
            break;
        case 0x04:
            // RLC H
            cpu.registers.H = cpu_rlc8(cpu.registers.H);
            break;
        case 0x05:
            // RLC L
            cpu.registers.L = cpu_rlc8(cpu.registers.L);
            break;
        case 0x06:
            // RLC (HL)
            value = bus_read8(cpu.registers.HL);
            result = cpu_rlc8(result);
            bus_write8(cpu.registers.HL, result);
        case 0x17:
            // RL A
            cpu.registers.A = cpu_rl8(cpu.registers.A);
            break;
        case 0x10:
            // RL B
            cpu.registers.B = cpu_rl8(cpu.registers.B);
            break;
        case 0x11:
            // RL C
            cpu.registers.C = cpu_rl8(cpu.registers.C);
            break;
        case 0x12:
            // RL D
            cpu.registers.D = cpu_rl8(cpu.registers.D);
            break;
        case 0x13:
            // RL E
            cpu.registers.E = cpu_rl8(cpu.registers.E);
            break;
        case 0x14:
            // RL H
            cpu.registers.H = cpu_rl8(cpu.registers.H);
            break;
        case 0x15:
            // RL L
            cpu.registers.L = cpu_rl8(cpu.registers.L);
            break;
        case 0x16:
            // RL (HL)
            value = bus_read8(cpu.registers.HL);
            result = cpu_rl8(value);
            bus_write8(cpu.registers.HL, result);
            break;
        default:
            assert(0);
            break;
    }
}

static void op_CE()
{
    uint8_t val = cpu_fetch8();
    cpu.registers.A = cpu_adc8(cpu.registers.A, val);
}

void op_D1()
{
    cpu.registers.E = cpu_read8(cpu.registers.SP);
    cpu.registers.SP++;
    cpu.registers.D = cpu_read8(cpu.registers.SP);
    cpu.registers.SP++;
}

void op_D6()
{
    uint8_t val = cpu_fetch8();
    cpu.registers.A = cpu_sub8(cpu.registers.A, val);
}

void op_DE()
{
    uint8_t val = cpu_fetch8();
    cpu.registers.A = cpu_sbc8(cpu.registers.A, val);
}

static void op_E0()
{
    uint8_t n = cpu_fetch8();
    bus_write8(0xFF00 + n, cpu.registers.A);
}

void op_E1()
{
    cpu.registers.L = cpu_read8(cpu.registers.SP);
    cpu.registers.SP++;
    cpu.registers.H = cpu_read8(cpu.registers.SP);
    cpu.registers.SP++;
}

static void op_E2()
{
    uint16_t address = 0xFF00 + cpu.registers.C;
    bus_write8(address, cpu.registers.A);
}

static void op_E5()
{
    cpu_idle();  /* internal M-cycle */

    cpu.registers.SP--;
    cpu_write8(cpu.registers.SP, cpu.registers.H);

    cpu.registers.SP--;
    cpu_write8(cpu.registers.SP, cpu.registers.L);
}

static void op_E6()
{
    uint8_t value = cpu_fetch8();
    cpu.registers.A = cpu_and8(value);
}

static void op_EA()
{
    uint8_t lo = cpu_fetch8();
    uint8_t hi = cpu_fetch8();
    uint16_t address = (hi << 8) | lo;
    bus_write8(address, cpu.registers.A);
}

static void op_E8()
{
    uint8_t imm = cpu_fetch8();

    uint16_t sp = cpu.registers.SP;

    cpu.registers.F = flags_add_sp_e8(sp, imm);

    cpu.registers.SP = sp + (int8_t)imm;

    cpu_idle();
    cpu_idle();
}

static void op_F0()
{
    uint8_t n = cpu_fetch8();
    cpu.registers.A = bus_read8(0xFF00+n);
}

static void op_F1()
{
    cpu.registers.F = cpu_read8(cpu.registers.SP) & 0xF0;
    cpu.registers.SP++;
    cpu.registers.A = cpu_read8(cpu.registers.SP);
    cpu.registers.SP++;
}

static void op_F2()
{
    uint16_t address = 0xFF00 + cpu.registers.C;
    cpu.registers.A = bus_read8(address);
}

void op_F3()
{
    cpu.ime = 0;
    cpu.ime_pending = 0;
}

static void op_F5()
{
    cpu_idle();  /* internal M-cycle */

    cpu.registers.SP--;
    cpu_write8(cpu.registers.SP, cpu.registers.A);

    cpu.registers.SP--;
    cpu_write8(cpu.registers.SP, cpu.registers.F & 0xF0);
}

static void op_F6()
{
    uint8_t value = cpu_fetch8();
    cpu_or8(value);
}

static void op_F8()
{
    uint8_t imm = cpu_fetch8();   /* consumes one M-cycle */

    cpu.registers.F = flags_add_sp_e8(cpu.registers.SP, imm);

    cpu.registers.HL = cpu.registers.SP + (int8_t)imm;

    cpu_idle();                   /* final internal M-cycle */
}

static void op_FA()
{
    uint8_t lo = cpu_fetch8();
    uint8_t hi = cpu_fetch8();
    uint16_t address = (hi << 8) | lo;
    cpu.registers.A = bus_read8(address);
}

void op_FB()
{
    cpu.ime_delay = 2;
}

static void op_FE()
{
    uint8_t value = cpu_fetch8();
    cpu_cp8(value);
}
