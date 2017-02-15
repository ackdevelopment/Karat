/* standard library functions */
#include<string.h>
/* karat files */
#include<vm/system.h>
#include<module.h>
#include<screen.h>
#include<alloc.h>
#include<log.h>

typedef struct mod *(*get_mod_f)(struct mod *);
/* so we don't need to deal with memory allocation */
struct builtin { get_mod_f get; struct mod m; };
#define BUILTIN(func) { .get = func }
#define SENTINEL BUILTIN(NULL)
static struct builtin builtins[] = {
	BUILTIN(screen_module),
	SENTINEL
};
#undef BUILTIN
#undef SENTINEL

void system_init(struct system *sys)
{
	err_on(!sys, "attempt to initialize unallocated struct");
	sys->mem = s_calloc(1, MEM_SIZE);
	memset(sys->ports, 0, sizeof(struct mod *) * IO_PORTS);
}

void system_load_builtins(struct system *sys)
{
	err_on(!sys, "uninitialized system");
	for(register size_t i = 0; i < IO_PORTS; ++i){
		struct builtin *b = &builtins[i];
		if(!b->get){
			break;
		}
		b->get(&b->m);
		debug("setting port %lu to %ls", i, b->m.name);
		system_set_port(sys, i, &b->m);
	}
}

void system_write(struct system *sys, addr_t addr, u8 data)
{
	err_on(!sys, "uninitialized system");
	err_on(addr >= MEM_SIZE, "address out of range");
	sys->mem[addr] = data;
}

u8 system_read(struct system *sys, addr_t addr)
{

	err_on(!sys, "uninitialized system");
	err_on(addr >= MEM_SIZE, "address out of range");
	return sys->mem[addr];
}

void system_io_write(struct system *sys, u8 which, u8 data)
{
	err_on(!sys, "uninitialized system");
	err_on(which >= IO_PORTS, "port outside of range");
	warn_ret(!sys->ports[which], "port not initialized");
	module_write(sys->ports[which], data);
}

u8 system_io_read(struct system *sys, u8 which)
{
	err_on(!sys, "uninitialized system");
	err_on(which >= IO_PORTS, "port outside of range");
	warn_ret(!sys->ports[which], "port not initialized") 0;
	return module_read(sys->ports[which]);
}

void system_set_port(struct system *sys, u8 which, struct mod *module)
{
	err_on(!sys, "uninitialized system");
	err_on(which >= IO_PORTS, "port outside of range");
	err_on(!module, "module in set_port not initialized");
	warn_on(sys->ports[which], "port %d already initialized to \"%ls\"", 
			which, sys->ports[which]->name);
	module_init(module);
	sys->ports[which] = module;
}

void system_reset_port(struct system *sys, u8 which)
{
	err_on(!sys, "uninitialized system");
	warn_on(which >= IO_PORTS, "port outside of range");
	if(which < IO_PORTS){
		if(sys->ports[which]){
			module_destroy(sys->ports[which]);
		}
		sys->ports[which] = NULL;
	}
}

void system_destroy(struct system *sys)
{
	warn_on(!sys, "attempt to destroy unallocated struct");
	if(sys){
		warn_on(!sys->mem, "memory not allocated");
		if(sys->mem){
			s_free(sys->mem);
		}
		for(size_t i = 0; i < IO_PORTS; ++i){
			if(sys->ports[i]){
				module_destroy(sys->ports[i]);
			}
		}
	}
}
