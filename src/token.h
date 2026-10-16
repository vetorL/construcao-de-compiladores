#ifndef COBOL_TOKEN_H
#define COBOL_TOKEN_H

#include <stddef.h>

typedef enum {
    TOK_EOF = 0,
#define KEYWORD(name, spelling) TOK_##name,
#include "keywords.def"
#undef KEYWORD
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_PERIOD,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_PICTURE_STRING,
    TOK_EQ,
    TOK_GT,
    TOK_LT,
    TOK_GE,
    TOK_LE,
    TOK_NE,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_POWER,
    TOK_SLASH,
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    char *message;
    size_t line;
    size_t column;
} Token;

void token_init(Token *token);
void token_destroy(Token *token);
const char *token_type_name(TokenType type);

#endif
