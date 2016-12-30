#include"module.h"

#include"types.h"
#include"mem.h"
#include"log.h"

void io_table_init(struct io_table *table)
{
	err_on(!table, "table not allocated");
	zmem(*table);
}

INTERNAL size_t find_handle(struct io_table *table, u8 code)
{
	err_on(!table, "table not allocated");
	if(code == 0){
		return MAX_HANDLES;
	}
	for(register size_t i = 0; i < MAX_HANDLES; ++i){
		if(!table->handles[i].code){
			return MAX_HANDLES;
		}else if(table->handles[i].code == code){
			return i;
		}
	}
	return MAX_HANDLES;
}

INTERNAL size_t find_empty(struct io_table *table, u8 code)
{
	err_on(!table, "table not allocated");
	if(code == 0){
		return MAX_HANDLES;
	}
	for(register size_t i = 0; i < MAX_HANDLES; ++i){
		if(!table->handles[i].code){
			return i;
		}
	}
	return MAX_HANDLES;

}

void io_table_add(struct io_table *table, u8 code, size_t args, io_cb cb)
{
	err_on(!table, "table not allocated");
	err_on(code == 0, "code cannot be 0x00");
	/* find an empty spot */
	size_t index = find_empty(table, code);
	err_on(index == MAX_HANDLES, "unable to add handle");
	/* add the new handler */
	table->handles[index] = (struct io_command){
		code,
		args,
		cb
	};
}

INTERNAL void arg_add(u8 **vec, size_t *size, size_t *cap, u8 val)
{
	if(*cap >= *size){
		*cap = (*cap) * 2 + 1;
		*vec = s_realloc(*vec, *cap);
	}
	(*vec)[(*size)++] = val;
}

void io_table_send(struct io_table *table, u8 byte)
{
	err_on(!table, "table not allocated");
	err_on((table->current == NULL) && (byte == 0), "commands may not be 0x00");
	/* start a new command */
	if(!table->current){
		size_t index = find_handle(table, byte);
		warn_ret(index == MAX_HANDLES, "attempt to start non-existent command");
		table->current = &table->handles[index];
	}else{
		struct arg_vec *vec = &table->cmdargs;
		arg_add(&vec->args, &vec->size, &vec->cap, byte);
	}
	/* check to see if we have the needed amount of arguments */
	if(table->cmdargs.size == table->current->nargs){
		CALL_CB(table->current->command, 
				table->cmdargs.args, table->cmdargs.size);
		table->current = NULL;
		if(table->cmdargs.args){
			table->cmdargs.size = 0;
			zmem(table->cmdargs);
		}
	}
}

void io_table_destroy(struct io_table *table)
{
	warn_on(!table, "table not allocated");
	if(table->cmdargs.args){
		s_free(table->cmdargs.args);
	}
}
