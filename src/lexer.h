#ifndef COBOL_LEXER_H
#define COBOL_LEXER_H

#include "token.h"

#include <stdio.h>

typedef struct Lexer Lexer;

Lexer *lexer_create(FILE *input, char **error_message);
void lexer_destroy(Lexer *lexer);
TokenType lexer_next(Lexer *lexer, Token *token);

#endif
