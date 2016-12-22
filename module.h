#pragma once

/* MODULE.H 
 *
 * Submodules may be added to the system 
 * to provide new functionality to the 
 * Karat system. Each module may use
 * io ports to communicate with the cpu.
 * code can use input to make decisions
 */

#include<stdint.h>
#include<wchar.h>

#include"karat.h"
#include"types.h"

struct mod {
	/* name of the module */
	const wchar_t *name;
	/* what to do when cpu writes to port */
	read_cb on_read;
	/* what to do when cpu reads from port */
	write_cb on_request;
};

struct mod *load_internal_module(struct mod *module, 
					read_cb on_read,
					write_cb on_write,
					const wchar_t *name);
struct mod *load_external_module(struct mod *module, const wchar_t *file);

void module_write(struct mod *module, uint8_t data);
uint8_t module_read(struct mod *module);
