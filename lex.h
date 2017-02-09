#pragma once
#include<wchar.h>
#include<stdlib.h>

#include"types.h"

struct token {
	#define MAX_LEXEME 50
	wchar_t lexeme[MAX_LEXEME];
	enum {
		TOK_ID,
		TOK_NUM,
		TOK_ADDR,
		TOK_REG,
		TOK_COLON,
		TOK_COMMA,
		TOK_EOL,
		TOK_EOS,
	} type;
	long long data;	/* for num and reg */
};

void lex_init(FILE *f);
int lex_next(struct token *tok);