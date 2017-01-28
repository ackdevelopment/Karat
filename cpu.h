#pragma once

#include<stdint.h>

#include"karat.h"
#include"types.h"

typedef uint32_t reg_t;	/* general register datatype */

#define GENERAL_REGS 31

struct cpu {
	/* general registers */
	union {
		reg_t regs[GENERAL_REGS];
		reg_t sp;
	};
	addr_t pc;
};

void cpu_init(struct cpu *state);
void cpu_reset(struct cpu *state);
