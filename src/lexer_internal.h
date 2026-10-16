#ifndef COBOL_LEXER_INTERNAL_H
#define COBOL_LEXER_INTERNAL_H

#include "fixed_format.h"
#include "token.h"

#include <stddef.h>

typedef struct {
    const SourceBuffer *source;
    size_t offset;
    size_t match_start;
    Token *token;
} LexerContext;

void lexer_note_match(LexerContext *context, size_t length);
TokenType lexer_emit(LexerContext *context, TokenType type,
                     const char *text, size_t length, const char *message);
TokenType lexer_emit_word(LexerContext *context, const char *text, size_t length);

#endif
