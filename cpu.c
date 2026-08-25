// CPU emulator
#include "cpu.h"
#include "memory.h"
#include "common.h"
#include "assert.h"
#include "stdio.h"

CPU_t cpu;

typedef void (*OpcodeHandler)(void);

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
    uint8_t opcode = cpu_read8(cpu.registers.PC); 
    printf("PC: %x opcode %x\n", cpu.registers.PC, opcode);
    cpu.registers.PC++; 
    return opcode; 
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
    extern OpcodeHandler opcode_table[];

    if (cpu.halted) 
    { 
        gb_tick(4); 
        //cpu_check_interrupts(); 
    }
    else
    {
        uint8_t opcode = cpu_fetch8(); 
        opcode_table[opcode](); 
        //cpu_check_interrupts(); 

        cpu_update_ime(); 
    }
} 

void debug()
{
    printf("-------------------------------\n");
    printf("AF = %02x\n", cpu.registers.AF);
    printf("BC = %02x\n", cpu.registers.BC);
    printf("DE = %02x\n", cpu.registers.DE);
    printf("HL = %02x\n", cpu.registers.HL);
    printf("PC = %02x\n", cpu.registers.PC);
    printf("SP = %02x\n", cpu.registers.SP);
}

void gb_run() 
{ 
    cpu.running = true;
    while(cpu.running) 
    { 
        debug();
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

static inline void cpu_rst(uint16_t vector)
{
    cpu_idle();  /* internal M-cycle */

    cpu.registers.SP--;
    cpu_write8(cpu.registers.SP, (cpu.registers.PC >> 8) & 0xFF);

    cpu.registers.SP--;
    cpu_write8(cpu.registers.SP, cpu.registers.PC & 0xFF);

    cpu.registers.PC = vector;
}

static inline void cpu_bit(uint8_t value, uint8_t bit)
{
    uint8_t carry = cpu.registers.F & FLAG_C;

    cpu.registers.F = carry | FLAG_H;

    if ((value & (1u << bit)) == 0)
        cpu.registers.F |= FLAG_Z;
}

static inline uint8_t cpu_rr8(uint8_t value)
{
    uint8_t old_carry = (cpu.registers.F & FLAG_C) ? 1 : 0;
    uint8_t new_carry = value & 0x01;

    uint8_t result = (value >> 1) | (old_carry << 7);

    cpu.registers.F = 0;

    if (result == 0)
        cpu.registers.F |= FLAG_Z;

    if (new_carry)
        cpu.registers.F |= FLAG_C;

    return result;
}

static inline uint8_t cpu_sla8(uint8_t value)
{
    uint8_t carry = (value >> 7) & 0x01;
    uint8_t result = value << 1;

    cpu.registers.F = 0;

    if (result == 0)
        cpu.registers.F |= FLAG_Z;

    if (carry)
        cpu.registers.F |= FLAG_C;

    return result;
}

static inline uint8_t cpu_sra8(uint8_t value)
{
    uint8_t carry = value & 0x01;
    uint8_t result = (value >> 1) | (value & 0x80);

    cpu.registers.F = 0;

    if (result == 0)
        cpu.registers.F |= FLAG_Z;

    if (carry)
        cpu.registers.F |= FLAG_C;

    return result;
}

static inline uint8_t cpu_srl8(uint8_t value)
{
    uint8_t carry = value & 0x01;
    uint8_t result = value >> 1;

    cpu.registers.F = 0;

    if (result == 0)
        cpu.registers.F |= FLAG_Z;

    if (carry)
        cpu.registers.F |= FLAG_C;

    return result;
}

void op_cb_bit(uint8_t opcode)
{
    uint8_t bit = (opcode >> 3) & 0x07;
    uint8_t reg = opcode & 0x07;

    uint8_t value;

    switch (reg) {
    case 0: value = cpu.registers.B; break;
    case 1: value = cpu.registers.C; break;
    case 2: value = cpu.registers.D; break;
    case 3: value = cpu.registers.E; break;
    case 4: value = cpu.registers.H; break;
    case 5: value = cpu.registers.L; break;
    case 6: value = cpu_read8(cpu.registers.HL); break;
    case 7: value = cpu.registers.A; break;
    default:
        return;
    }

    cpu_bit(value, bit);
}

static inline uint8_t cpu_set_bit(uint8_t value, uint8_t bit)
{
    return value | (1u << bit);
}

static inline uint8_t cpu_res_bit(uint8_t value, uint8_t bit)
{
    return value & ~(1u << bit);
}

void op_cb_set(uint8_t opcode)
{
    uint8_t bit = (opcode >> 3) & 0x07;
    uint8_t reg = opcode & 0x07;

    switch (reg) {
    case 0:
        cpu.registers.B = cpu_set_bit(cpu.registers.B, bit); break;
    case 1:
        cpu.registers.C = cpu_set_bit(cpu.registers.C, bit); break;
    case 2:
        cpu.registers.D = cpu_set_bit(cpu.registers.D, bit); break;
    case 3:
        cpu.registers.E = cpu_set_bit(cpu.registers.E, bit); break;
    case 4:
        cpu.registers.H = cpu_set_bit(cpu.registers.H, bit); break;
    case 5:
        cpu.registers.L = cpu_set_bit(cpu.registers.L, bit); break;
    case 6: {
        uint8_t value = cpu_read8(cpu.registers.HL);
        value = cpu_set_bit(value, bit);
        cpu_write8(cpu.registers.HL, value);
        break;
    }
    case 7:
        cpu.registers.A = cpu_set_bit(cpu.registers.A, bit); break;
    }
}

void op_cb_res(uint8_t opcode)
{
    uint8_t bit = (opcode >> 3) & 0x07;
    uint8_t reg = opcode & 0x07;

    switch (reg) {
    case 0:
        cpu.registers.B = cpu_res_bit(cpu.registers.B, bit); break;
    case 1:
        cpu.registers.C = cpu_res_bit(cpu.registers.C, bit); break;
    case 2:
        cpu.registers.D = cpu_res_bit(cpu.registers.D, bit); break;
    case 3:
        cpu.registers.E = cpu_res_bit(cpu.registers.E, bit); break;
    case 4:
        cpu.registers.H = cpu_res_bit(cpu.registers.H, bit); break;
    case 5:
        cpu.registers.L = cpu_res_bit(cpu.registers.L, bit); break;
    case 6: {
        uint8_t value = cpu_read8(cpu.registers.HL);
        value = cpu_res_bit(value, bit);
        cpu_write8(cpu.registers.HL, value);
        break;
    }
    case 7:
        cpu.registers.A = cpu_res_bit(cpu.registers.A, bit); break;
    }
}

static inline uint16_t cpu_fetch16()
{
    uint8_t lo = cpu_fetch8();
    uint8_t hi = cpu_fetch8();

    return ((uint16_t)hi << 8) | lo;
}

static inline void cpu_jp_cond(bool_t condition)
{
    uint16_t addr = cpu_fetch16();

    if (condition) {
        cpu_idle();
        cpu.registers.PC = addr;
    }
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

static void op_07() 
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

static void op_0E() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.C = val; 
} 

static void op_0F() 
{ 
    uint8_t a = cpu.registers.A; 
    uint8_t carry = a & 0x01; 

    cpu.registers.A = (a >> 1) | (carry << 7); 

    cpu.registers.F = carry ? FLAG_C : 0; 
} 

// TBD 
static void op_10() 
{ 
    assert(0);
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
    bus_write8(address, cpu.registers.A); 
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

static void op_17() 
{ 
    uint8_t a = cpu.registers.A; 
    uint8_t old_carry = (cpu.registers.F & FLAG_C) ? 1 : 0; 
    uint8_t new_carry = (a >> 7) & 1; 

    cpu.registers.A = (a << 1) | old_carry; 

    cpu.registers.F = new_carry ? FLAG_C : 0; 
} 

static void op_18()
{
    assert(0);

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

static void op_1E() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.E = val; 
} 

static void op_1F() 
{ 
    uint8_t a = cpu.registers.A; 
    uint8_t old_carry = (cpu.registers.F & FLAG_C) ? 1 : 0; 
    uint8_t new_carry = a & 0x01; 

    cpu.registers.A = (a >> 1) | (old_carry << 7); 

    cpu.registers.F = new_carry ? FLAG_C : 0; 
} 

static void op_20()
{
    int8_t offset = cpu_fetch8();

    if (!(cpu.registers.F & FLAG_Z)) {
        cpu.registers.PC = (uint16_t)(cpu.registers.PC + offset);
        cpu_idle();
    }
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
    unsigned char carry = (cpu.registers.F & FLAG_C) != 0; 

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

static void op_28()
{
    assert(0);

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

static void op_2E() 
{ 
    uint8_t val = cpu_fetch8(); 
    cpu.registers.L = val; 
} 

static void op_2F()
{
    assert(0);

}

static void op_30() 
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

static void op_36()
{
    assert(0);

}

static void op_37() 
{ 
    /* Preserve Z, clear N/H, set C */ 
    cpu.registers.F = (cpu.registers.F & FLAG_Z) | FLAG_C; 
} 

static void op_38()
{
    assert(0);

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

static void op_3F() 
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

static void op_4F()
{

    assert(0);
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
    assert(0);
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

static void op_90() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.B); 
} 

static void op_91() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.C); 
} 

static void op_92() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.D); 
} 

static void op_93() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.E); 
} 

static void op_94() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.H); 
} 

static void op_95() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.L); 
} 

static void op_96() 
{ 
    uint8_t val = bus_read8(cpu.registers.HL); 
    cpu.registers.A = cpu_sub8(cpu.registers.A, val); 
} 

static void op_97() 
{ 
    cpu.registers.A = cpu_sub8(cpu.registers.A, cpu.registers.A); 
} 

static void op_98() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.B); 
} 

static void op_99() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.C); 
} 

static void op_9A() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.D); 
} 

static void op_9B() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.E); 
} 

static void op_9C() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.H); 
} 

static void op_9D() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.L); 
} 

static void op_9E() 
{ 
    uint8_t val = bus_read8(cpu.registers.HL); 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, val); 
} 

static void op_9F() 
{ 
    cpu.registers.A = cpu_sbc8(cpu.registers.A, cpu.registers.A); 
} 

static void op_A0() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.B); 
} 

static void op_A1() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.C); 
} 

static void op_A2() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.D); 
} 

static void op_A3() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.E); 
} 

static void op_A4() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.H); 
} 

static void op_A5() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.L); 
} 

static void op_A6() 
{ 
    uint8_t value = bus_read8(cpu.registers.HL); 
    cpu.registers.A = cpu_and8(value); 
} 

static void op_A7() 
{ 
    cpu.registers.A = cpu_and8(cpu.registers.A); 
} 

static void op_A8() 
{ 
    cpu_xor8(cpu.registers.B); 
} 

static void op_A9() 
{ 
    cpu_xor8(cpu.registers.C); 
} 

static void op_AA() 
{ 
    cpu_xor8(cpu.registers.D); 
} 

static void op_AB() 
{ 
    cpu_xor8(cpu.registers.E); 
} 

static void op_AC() 
{ 
    cpu_xor8(cpu.registers.H); 
} 

static void op_AD() 
{ 
    cpu_xor8(cpu.registers.L); 
} 

static void op_AE() 
{ 
    uint8_t value = bus_read8(cpu.registers.HL); 
    cpu_xor8(value); 
} 

static void op_AF() 
{ 
    cpu_xor8(cpu.registers.A); 
} 

static void op_B0() 
{ 
    cpu_or8(cpu.registers.B); 
} 

static void op_B1() 
{ 
    cpu_or8(cpu.registers.C); 
} 

static void op_B2() 
{ 
    cpu_or8(cpu.registers.D); 
} 

static void op_B3() 
{ 
    cpu_or8(cpu.registers.E); 
} 

static void op_B4() 
{ 
    cpu_or8(cpu.registers.H); 
} 

static void op_B5() 
{ 
    cpu_or8(cpu.registers.L); 
} 

static void op_B6() 
{ 
    uint8_t value = bus_read8(cpu.registers.HL); 
    cpu_or8(value); 
} 

static void op_B7() 
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

static void op_BF()
{
    assert(0);

}

static void op_C0()
{
    assert(0);

}

static void op_C1()
{
    assert(0);

}

/* C2: JP NZ,nn */
static void op_C2()
{
    cpu_jp_cond(!(cpu.registers.F & FLAG_Z));
}

void op_C3()
{
    uint8_t lo = cpu_fetch8();
    uint8_t hi = cpu_fetch8();

    uint16_t addr = ((uint16_t)hi << 8) | lo;

    cpu_idle();

    cpu.registers.PC = addr;
}

static void op_C4()
{
    assert(0);

}

static void op_C5() 
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

static void op_C7()
{
    cpu_rst(0x0000);
}

static void op_C8()
{
    assert(0);

}

static void op_C9()
{
    assert(0);

}

/* CA: JP Z,nn */
void op_CA()
{
    cpu_jp_cond(cpu.registers.F & FLAG_Z);
}

static void op_CB() 
{ 
    uint8_t subcode = cpu_fetch8(); 
    uint8_t value, result; 
    switch(subcode) 
    { 
        case 0x08:
            cpu.registers.B = cpu_rrc8(cpu.registers.B);
            break;
        case 0x09:
            cpu.registers.C = cpu_rrc8(cpu.registers.C);
            break;
        case 0x0A:
            cpu.registers.D = cpu_rrc8(cpu.registers.D);
            break;
        case 0x0B:
            cpu.registers.E = cpu_rrc8(cpu.registers.E);
            break;
        case 0x0C:
            cpu.registers.H = cpu_rrc8(cpu.registers.H);
            break;
        case 0x0D:
            cpu.registers.L = cpu_rrc8(cpu.registers.L);
            break;
        case 0x0E: 
            value = bus_read8(cpu.registers.HL); 
            result = cpu_rrc8(value); 
            bus_write8(cpu.registers.HL, result); 
            break;
        case 0x0f:
            cpu.registers.A = cpu_rrc8(cpu.registers.A);
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
        case 0x37:
            cpu.registers.A = cpu_swap8(cpu.registers.A);
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
            result = cpu_rlc8(value);
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
        case 0x1F:
            cpu.registers.A = cpu_rr8(cpu.registers.A);
            break;
        case 0x18:
            cpu.registers.B = cpu_rr8(cpu.registers.B);
            break;
        case 0x19:
            cpu.registers.C = cpu_rr8(cpu.registers.C);
            break;
        case 0x1A:
            cpu.registers.D = cpu_rr8(cpu.registers.D);
            break;
        case 0x1B:
            cpu.registers.E = cpu_rr8(cpu.registers.E);
            break;
        case 0x1C:
            cpu.registers.H = cpu_rr8(cpu.registers.H);
            break;
        case 0x1D:
            cpu.registers.L = cpu_rr8(cpu.registers.L);
            break;
        case 0x1E:
            value = bus_read8(cpu.registers.HL);
            result = cpu_rr8(value);
            bus_write8(cpu.registers.HL, result);
            break;
        case 0x27:
            cpu.registers.A = cpu_sla8(cpu.registers.A);
            break;
        case 0x20:
            cpu.registers.B = cpu_sla8(cpu.registers.B);
            break;
        case 0x21:
            cpu.registers.C = cpu_sla8(cpu.registers.C);
            break;
        case 0x22:
            cpu.registers.D = cpu_sla8(cpu.registers.D);
            break;
        case 0x23:
            cpu.registers.E = cpu_sla8(cpu.registers.E);
            break;
        case 0x24:
            cpu.registers.H = cpu_sla8(cpu.registers.H);
            break;
        case 0x25:
            cpu.registers.L = cpu_sla8(cpu.registers.L);
            break;
        case 0x26:
            value = bus_read8(cpu.registers.HL);
            bus_write8(cpu.registers.HL, cpu_sla8(cpu.registers.A));
            break;
        case 0x2F:
            cpu.registers.A = cpu_sra8(cpu.registers.A);
            break;
        case 0x28:
            cpu.registers.B = cpu_sra8(cpu.registers.B);
            break;
        case 0x29:
            cpu.registers.C = cpu_sra8(cpu.registers.C);
            break;
        case 0x2A:
            cpu.registers.D = cpu_sra8(cpu.registers.D);
            break;
        case 0x2B:
            cpu.registers.E = cpu_sra8(cpu.registers.E);
            break;
        case 0x2C:
            cpu.registers.H = cpu_sra8(cpu.registers.H);
            break;
        case 0x2D:
            cpu.registers.L = cpu_sra8(cpu.registers.L);
            break;
        case 0x2E:
            value = bus_read8(cpu.registers.HL);
            bus_write8(cpu.registers.HL, cpu_sra8(value));
            break;
        case 0x3F:
            cpu.registers.A = cpu_srl8(cpu.registers.A);
            break;
        case 0x38:
            cpu.registers.B = cpu_srl8(cpu.registers.B);
            break;
        case 0x39:
            cpu.registers.C = cpu_srl8(cpu.registers.C);
            break;
        case 0x3A:
            cpu.registers.D = cpu_srl8(cpu.registers.D);
            break;
        case 0x3B:
            cpu.registers.E = cpu_srl8(cpu.registers.E);
            break;
        case 0x3C:
            cpu.registers.H = cpu_srl8(cpu.registers.H);
            break;
        case 0x3D:
            cpu.registers.L = cpu_srl8(cpu.registers.L);
            break;
        case 0x3E:
            value = bus_read8(cpu.registers.HL);
            bus_write8(cpu.registers.HL, cpu_srl8(value));
            break;
        case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
        case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
        case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
        case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
        case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
        case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
        case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            op_cb_bit(subcode);
            break;
        case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
        case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F:
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
        case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F:
        case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7:
        case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
        case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
        case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
            op_cb_res(subcode);
            break;
        case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC6: case 0xC7:
        case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF:
        case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7:
        case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
        case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xE7:
        case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEE: case 0xEF:
        case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF7:
        case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFE: case 0xFF:
            op_cb_set(subcode);
            break;
        default:
            printf("subcode = %x\n", subcode);
            assert(0);
            break;
    }
}

static void op_CC()
{
    assert(0);
}

static void op_CD()
{
    assert(0);

}

static void op_CE()
{
    uint8_t val = cpu_fetch8();
    cpu.registers.A = cpu_adc8(cpu.registers.A, val);
}

static void op_CF()
{
    cpu_rst(0x0008);
}

static void op_D0()
{
    assert(0);

}

static void op_D1()
{
    cpu.registers.E = cpu_read8(cpu.registers.SP);
    cpu.registers.SP++;
    cpu.registers.D = cpu_read8(cpu.registers.SP);
    cpu.registers.SP++;
}

/* D2: JP NC,nn */
static void op_D2()
{
    cpu_jp_cond(!(cpu.registers.F & FLAG_C));
}

static void op_D3()
{
    assert(0);

}

static void op_D4()
{
    assert(0);

}

static void op_D5()
{
    assert(0);

}

static void op_D6()
{
    uint8_t val = cpu_fetch8();
    cpu.registers.A = cpu_sub8(cpu.registers.A, val);
}

static void op_D7()
{
    cpu_rst(0x0010);
}

static void op_D8()
{
    assert(0);

}

static void op_D9()
{
    assert(0);

}

/* DA: JP C,nn */
static void op_DA()
{
    cpu_jp_cond(cpu.registers.F & FLAG_C);
}

static void op_DB()
{
    assert(0);

}

static void op_DC()
{
    assert(0);

}

static void op_DD()
{
    assert(0);

}

static void op_DE()
{
    uint8_t val = cpu_fetch8();
    cpu.registers.A = cpu_sbc8(cpu.registers.A, val);
}

static void op_DF()
{
    cpu_rst(0x0018);
}

static void op_E0()
{
    uint8_t n = cpu_fetch8();
    bus_write8(0xFF00 + n, cpu.registers.A);
}

static void op_E1()
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

static void op_E3()
{
    assert(0);

}

static void op_E4()
{
    assert(0);

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

static void op_E7()
{
    cpu_rst(0x0020);
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

static void op_E9()
{
    assert(0);

}

static void op_EA()
{
    uint8_t lo = cpu_fetch8();
    uint8_t hi = cpu_fetch8();
    uint16_t address = (hi << 8) | lo;
    bus_write8(address, cpu.registers.A);
}

static void op_EB()
{
    assert(0);

}

static void op_EC()
{
    assert(0);

}

static void op_ED()
{
    assert(0);

}

static void op_EE()
{
    assert(0);

}

static void op_EF()
{
    cpu_rst(0x0028);
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

static void op_F3()
{
    cpu.ime = 0;
    cpu.ime_pending = 0;
}

static void op_F4()
{
    assert(0);

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

static void op_F7()
{
    cpu_rst(0x0030);
}

static void op_F8()
{
    uint8_t imm = cpu_fetch8();   /* consumes one M-cycle */

    cpu.registers.F = flags_add_sp_e8(cpu.registers.SP, imm);

    cpu.registers.HL = cpu.registers.SP + (int8_t)imm;

    cpu_idle();                   /* final internal M-cycle */
}

static void op_F9()
{
    assert(0);

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

void op_FC()
{
    assert(0);

}

void op_FD()
{
    assert(0);

}

static void op_FE()
{
    uint8_t value = cpu_fetch8();
    cpu_cp8(value);
}

static void op_FF()
{
    cpu_rst(0x0038);
}

OpcodeHandler opcode_table[] = {
    op_00, op_01, op_02, op_03, op_04, op_05, op_06, op_07,
    op_08, op_09, op_0A, op_0B, op_0C, op_0D, op_0E, op_0F,
    op_10, op_11, op_12, op_13, op_14, op_15, op_16, op_17,
    op_18, op_19, op_1A, op_1B, op_1C, op_1D, op_1E, op_1F,
    op_20, op_21, op_22, op_23, op_24, op_25, op_26, op_27,
    op_28, op_29, op_2A, op_2B, op_2C, op_2D, op_2E, op_2F,
    op_30, op_31, op_32, op_33, op_34, op_35, op_36, op_37,
    op_38, op_39, op_3A, op_3B, op_3C, op_3D, op_3E, op_3F,
    op_40, op_41, op_42, op_43, op_44, op_45, op_46, op_47,
    op_48, op_49, op_4A, op_4B, op_4C, op_4D, op_4E, op_4F,
    op_50, op_51, op_52, op_53, op_54, op_55, op_56, op_57,
    op_58, op_59, op_5A, op_5B, op_5C, op_5D, op_5E, op_5F,
    op_60, op_61, op_62, op_63, op_64, op_65, op_66, op_67,
    op_68, op_69, op_6A, op_6B, op_6C, op_6D, op_6E, op_6F,
    op_70, op_71, op_72, op_73, op_74, op_75, op_76, op_77,
    op_78, op_79, op_7A, op_7B, op_7C, op_7D, op_7E, op_7F,
    op_80, op_81, op_82, op_83, op_84, op_85, op_86, op_87,
    op_88, op_89, op_8A, op_8B, op_8C, op_8D, op_8E, op_8F,
    op_90, op_91, op_92, op_93, op_94, op_95, op_96, op_97,
    op_98, op_99, op_9A, op_9B, op_9C, op_9D, op_9E, op_9F,
    op_A0, op_A1, op_A2, op_A3, op_A4, op_A5, op_A6, op_A7,
    op_A8, op_A9, op_AA, op_AB, op_AC, op_AD, op_AE, op_AF,
    op_B0, op_B1, op_B2, op_B3, op_B4, op_B5, op_B6, op_B7,
    op_B8, op_B9, op_BA, op_BB, op_BC, op_BD, op_BE, op_BF,
    op_C0, op_C1, op_C2, op_C3, op_C4, op_C5, op_C6, op_C7,
    op_C8, op_C9, op_CA, op_CB, op_CC, op_CD, op_CE, op_CF,
    op_D0, op_D1, op_D2, op_D3, op_D4, op_D5, op_D6, op_D7,
    op_D8, op_D9, op_DA, op_DB, op_DC, op_DD, op_DE, op_DF,
    op_E0, op_E1, op_E2, op_E3, op_E4, op_E5, op_E6, op_E7,
    op_E8, op_E9, op_EA, op_EB, op_EC, op_ED, op_EE, op_EF,
    op_F0, op_F1, op_F2, op_F3, op_F4, op_F5, op_F6, op_F7,
    op_F8, op_F9, op_FA, op_FB, op_FC, op_FD, op_FE, op_FF};
