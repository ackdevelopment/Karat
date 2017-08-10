#pragma once
/* stdlib files */
#include<stdint.h>
#include<stdio.h>
/* karat files */
#include<vm/kprog.h>
#include<ktypes.h>

typedef uint32_t reg_t; /* general register datatype */

#define GENERAL_REGS 32

struct cpu {
    /* general registers */
    union {
        reg_t regs[GENERAL_REGS];
        struct {
            reg_t __pad[GENERAL_REGS - 3];
            reg_t sp;
            reg_t bp;
            reg_t pc;
        };
    };
};

void cpu_init(struct cpu *state);
void cpu_reset(struct cpu *state);
void cpu_step(struct cpu *state, struct kprog *prog);
void cpu_run(struct cpu *cpu, struct kprog *prog);
