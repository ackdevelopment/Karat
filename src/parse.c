#include<stdio.h>
#include<wctype.h>
#include<ctype.h>
#include<wchar.h>

#include"parse/parse.h"
#include"parse/lex.h"
#include"util.h"
#include"alloc.h"
#include"log.h"
#include"smap.h"

#define BYTE_SIZE 1
#define WORD_SIZE 2
#define DWORD_SIZE 4

typedef i8 byte_t ;
typedef i16 word_t;
typedef i32 dword_t;

#define MAX_LOOK 10 

struct fwdef {
	SLINK(struct fwdef);	/* next forward declaration in list */
	addr_t *rpos;			/* location to resolve */
};

struct label_def {
	struct fwdef *fwdefs;
	addr_t pos;				/* what position it points to */
};

/* deal with all forward declarations */
static void resolve_fwdefs(struct label_def *def)
{
	/* this will destroy the list */
	for(struct fwdef *d = def->fwdefs, *next = NULL; d; d = next){
		next = d->next;
		s_free(d);
	}
	def->fwdefs = NULL;
}

struct parse_state {
	/* parsing utils */
	struct lex_state lstate;
	/* MAX_LOOK tokens will always be in buffer */
	struct token la_buff[MAX_LOOK];
	size_t cur_size;
	size_t index;
	/* for generation */
	struct kprog *prog;	/* output program */
	size_t cur_insns;	/* current position in bytecode */
	/* map of labels to addresses */ 
	struct smap *label_defs;
};

static inline void prog_write(struct parse_state *state, i32 data, size_t bytes)
{
	(void) state;
	register const u8 *bp = (u8 *)&data;
	for(size_t i = 1; i <= bytes; ++i){
		printf("0x%X\n", bp[bytes - i]);
	}
}

static void CONSTRUCTOR test_prog_write()
{
	long long data = 0xffeeaabb;
	prog_write(NULL, data, 4);
}

/* get the actual index of the nth element in the buffer */
static inline size_t rbuff_elem_index(struct parse_state *state, size_t n)
{
	return (state->index + n) % MAX_LOOK;
}

static struct token *rbuff_elem(struct parse_state *state, size_t n)
{
	return &state->la_buff[rbuff_elem_index(state, n)];
}

/* add an element to the end of the buffer */
static inline void rbuff_push_back(struct parse_state *state)
{
	lex_next(&state->lstate, rbuff_elem(state, state->cur_size));
	++state->cur_size;
}

/* remove an element from the end of the buffer */
static inline void rbuff_pop_back(struct parse_state *state)
{
	--state->cur_size;
}

/* remove an element from the front of the buffer */
static inline void rbuff_pop_front(struct parse_state *state)
{
	state->index = rbuff_elem_index(state, 1);
	--state->cur_size;
}

static void parse_init(struct parse_state *state, struct kprog *prog, FILE *f)
{
	memset(state->la_buff, 0, sizeof(struct token)*MAX_LOOK);
	state->cur_size = 0;
	state->index = 0;

	lex_init(&state->lstate, f);

	for(size_t i = 0; i < MAX_LOOK; ++i){
		rbuff_push_back(state);
	}

	state->prog = prog;
	state->cur_insns = 0;
	state->label_defs = smap_create_d();
}

static void parse_destroy(struct parse_state *ctx)
{
	smap_destroy(ctx->label_defs);
}

static int parse_test_n(struct parse_state *state, size_t n, unsigned tok_type)
{
	if(n >= MAX_LOOK){
		return 0;
	}
	return rbuff_elem(state, n)->type == tok_type;
}

static int parse_test(struct parse_state *state, unsigned tok_type)
{
	return parse_test_n(state, 0, tok_type);
}

static void parse_advance(struct parse_state *state)
{
	/* delete front element from buffer */
	rbuff_pop_front(state);
	/* read new element into buffer */
	rbuff_push_back(state);
}

static const struct token *parse_la_n(struct parse_state *state, size_t n)
{
	(void) parse_test;
	return rbuff_elem(state, n);
}

static const struct token *parse_la(struct parse_state *state)
{
	return parse_la_n(state, 0);
}

static void parse_match(struct parse_state *state, unsigned tok_type)
{
	if(parse_la(state)->type != tok_type){
		err("couldn't match target type on \"%ls\"", parse_la(state)->lexeme);
	}else{
		parse_advance(state);
	}
}

static inline void add_fwdef(struct label_def *def, addr_t *pos)
{
	struct fwdef *nfwdef = s_alloc(struct fwdef);
	nfwdef->next = def->fwdefs;
	nfwdef->rpos = pos;
	def->fwdefs = nfwdef;
}

static struct label_def *add_label_def(struct smap *smap, const wchar_t *lexeme, addr_t pos)
{
	struct label_def *def = s_alloc(struct label_def);
	def->fwdefs = NULL;
	def->pos = pos;
	smap_insert(smap, lexeme, def);
	return def;
}

static int parse_label(struct parse_state *state)
{
	const wchar_t *tmp = parse_la(state)->lexeme;
	wchar_t label[wcslen(tmp) + 1];
	wcscpy(label, tmp);

	parse_match(state, TOK_ID);
	parse_match(state, TOK_COLON);
	printf("encountered label %ls:\n", label);
	if(!wcscmp(label, KPROG_ENTRY_POINT)){
		if(state->prog->entry_point){
			err("multiple definitions of entry point");
			return 1;
		}else{
			state->prog->entry_point = state->cur_insns;
		}
	}
	struct label_def *def = smap_lookup(state->label_defs, label);
	if(def){
		if(def->fwdefs){
			puts("resolving forward definitions");
			resolve_fwdefs(def);
		}else{
			printf("label %ls redefined at %lu\n", label, state->lstate.line_no);
			return 1;
		}
	}else{	/* no label yet */
		add_label_def(state->label_defs, label, state->cur_insns);
	}
	return 0;
}

static int parse_arg(struct parse_state *state)
{
	const struct token *la = parse_la(state);
	switch(la->type){
	case TOK_REG:	/* register argument */
	case TOK_NUM:	/* number argument */
	case TOK_ADDR:
		parse_advance(state);
		break;
	case TOK_ID:	/* label argument */
	{
		struct label_def *def = NULL;

		/* check if there is already a symbol table entry */
		if((def = smap_lookup(state->label_defs, la->lexeme))){
			/* check if not defined */
			if(def->fwdefs){
				puts("forward declaration");
				add_fwdef(def, (addr_t *)&state->prog->program[state->cur_insns]);
			}else{
				addr_t pos = def->pos;
				printf("label argument %d\n", pos);
			}
		}else{
			/* add to table */
			struct label_def *ndef = add_label_def(state->label_defs, la->lexeme, state->cur_insns);
			add_fwdef(ndef, (addr_t *)&state->prog->program[state->cur_insns]);
		}
		parse_advance(state);
		break;
	}
	case TOK_EOL:
	default:
		return 1;
	};
	return 0;
}

static int parse_args(struct parse_state *state)
{
	while(!parse_arg(state)){
		if(parse_test(state, TOK_COMMA)){
			parse_match(state, TOK_COMMA);
		}else{
			break;
		}
	}
	return 0;
}

static int parse_ins(struct parse_state *state)
{
	/* I should probably duplicate this string */
	const wchar_t *op_str = parse_la(state)->lexeme;
	printf("opcode %ls\n", op_str);
	parse_match(state, TOK_ID);
	++state->cur_insns;
	return parse_args(state);
}

static int parse_expr(struct parse_state *state)
{
	switch(parse_la(state)->type){
	case TOK_ID:
		if(parse_test_n(state, 1, TOK_COLON)){
			if(parse_label(state)){
				return 1;
			}else{
				return parse_expr(state);
			}
		}else{
			if(parse_ins(state)){
				return 1;
			}
			parse_match(state, TOK_EOL);
			return 0;
		}
	case TOK_EOL:
		parse_advance(state);
		return 0;
	default:
		return 1;
	};
}

int parse_file(FILE *f, struct kprog *res)
{
	err_on(!f, "input file not opened");
	err_on(!res, "output program not allocated");

	struct parse_state state;
	parse_init(&state, res, f);

	int ret = 0;
	const struct token *la = parse_la(&state);
	while(la->type != TOK_EOS){
		ret = parse_expr(&state);
		if(ret){
			break;
		}
		la = parse_la(&state);
	}
	/* TODO check_fwdefs(state) */
	parse_destroy(&state);
	return ret;
}
