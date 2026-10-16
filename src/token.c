#include "token.h"

#include <stdlib.h>

void token_init(Token *token) {
    token->type = TOK_EOF;
    token->lexeme = NULL;
    token->message = NULL;
    token->line = 0;
    token->column = 0;
}

void token_destroy(Token *token) {
    free(token->lexeme);
    free(token->message);
    token_init(token);
}

const char *token_type_name(TokenType type) {
    if (type == TOK_KW_ERROR) return "KW_ERROR";
    if (type == TOK_KW_STRING) return "KW_STRING";
    static const char *const names[] = {
        "EOF",
#define KEYWORD(name, spelling) spelling,
#include "keywords.def"
#undef KEYWORD
        "IDENTIFIER", "NUMBER", "STRING",
        "PERIOD", "COMMA", "SEMICOLON", "LPAREN", "RPAREN",
        "PICTURE_STRING", "EQ", "GT", "LT", "GE", "LE", "NE",
        "PLUS", "MINUS", "STAR", "POWER", "SLASH", "ERROR"
    };
    size_t count = sizeof(names) / sizeof(names[0]);
    return (size_t)type < count ? names[type] : "UNKNOWN";
}
