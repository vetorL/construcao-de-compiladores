#include "lexer.h"

#include "fixed_format.h"
#include "lexer_internal.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef void *yyscan_t;
typedef struct yy_buffer_state *YY_BUFFER_STATE;

int cobol_yylex_init_extra(LexerContext *extra, yyscan_t *scanner);
int cobol_yylex_destroy(yyscan_t scanner);
YY_BUFFER_STATE cobol_yy_scan_bytes(const char *bytes, int length, yyscan_t scanner);
void cobol_yy_delete_buffer(YY_BUFFER_STATE buffer, yyscan_t scanner);
int cobol_yylex(yyscan_t scanner);

struct Lexer {
    SourceBuffer source;
    LexerContext context;
    yyscan_t scanner;
    YY_BUFFER_STATE scan_buffer;
};

static char *duplicate_n(const char *text, size_t length) {
    char *copy = malloc(length + 1);
    if (copy != NULL) {
        memcpy(copy, text, length);
        copy[length] = '\0';
    }
    return copy;
}

void lexer_note_match(LexerContext *context, size_t length) {
    context->match_start = context->offset;
    context->offset += length;
}

TokenType lexer_emit(LexerContext *context, TokenType type,
                     const char *text, size_t length, const char *message) {
    Token *token = context->token;
    token->type = type;
    token->lexeme = duplicate_n(text, length);
    token->message = message == NULL ? NULL : duplicate_n(message, strlen(message));
    if (context->match_start < context->source->length) {
        SourceLocation location = context->source->locations[context->match_start];
        token->line = location.line;
        token->column = location.column;
    } else if (context->source->length > 0) {
        SourceLocation location = context->source->locations[context->source->length - 1];
        token->line = location.line;
        token->column = location.column;
    } else {
        token->line = token->column = 1;
    }
    if (token->lexeme == NULL || (message != NULL && token->message == NULL)) {
        free(token->message);
        token->message = duplicate_n("out of memory while creating token", 34);
        token->type = TOK_ERROR;
    }
    return token->type;
}

static int equals_keyword(const char *text, size_t length, const char *keyword) {
    if (strlen(keyword) != length) {
        return 0;
    }
    for (size_t i = 0; i < length; i++) {
        if (toupper((unsigned char)text[i]) != keyword[i]) {
            return 0;
        }
    }
    return 1;
}

TokenType lexer_emit_word(LexerContext *context, const char *text, size_t length) {
    static const struct {
        const char *word;
        TokenType type;
    } keywords[] = {
#define KEYWORD(name, spelling) {spelling, TOK_##name},
#include "keywords.def"
#undef KEYWORD
    };

    int all_digits = 1;
    for (size_t i = 0; i < length; i++) {
        if (!isdigit((unsigned char)text[i])) {
            all_digits = 0;
            break;
        }
    }
    if (all_digits) {
        return lexer_emit(context, TOK_NUMBER, text, length, NULL);
    }
    if (length > 30) {
        return lexer_emit(context, TOK_ERROR, text, length,
                          "identifier exceeds the 30-character limit");
    }
    if (text[length - 1] == '-') {
        return lexer_emit(context, TOK_ERROR, text, length,
                          "identifier cannot end with '-'");
    }
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (equals_keyword(text, length, keywords[i].word)) {
            return lexer_emit(context, keywords[i].type, text, length, NULL);
        }
    }
    return lexer_emit(context, TOK_IDENTIFIER, text, length, NULL);
}

Lexer *lexer_create(FILE *input, char **error_message) {
    *error_message = NULL;
    Lexer *lexer = calloc(1, sizeof(*lexer));
    if (lexer == NULL) {
        return NULL;
    }
    source_buffer_init(&lexer->source);
    if (!fixed_format_read(input, &lexer->source, error_message)) {
        lexer_destroy(lexer);
        return NULL;
    }
    if (lexer->source.length > INT_MAX - 2) {
        *error_message = duplicate_n("source exceeds scanner size limit", 33);
        lexer_destroy(lexer);
        return NULL;
    }
    lexer->context.source = &lexer->source;
    if (cobol_yylex_init_extra(&lexer->context, &lexer->scanner) != 0) {
        lexer_destroy(lexer);
        return NULL;
    }
    lexer->scan_buffer = cobol_yy_scan_bytes(lexer->source.text,
                                             (int)lexer->source.length,
                                             lexer->scanner);
    if (lexer->scan_buffer == NULL) {
        lexer_destroy(lexer);
        return NULL;
    }
    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    if (lexer == NULL) {
        return;
    }
    if (lexer->scan_buffer != NULL) {
        cobol_yy_delete_buffer(lexer->scan_buffer, lexer->scanner);
    }
    if (lexer->scanner != NULL) {
        cobol_yylex_destroy(lexer->scanner);
    }
    source_buffer_destroy(&lexer->source);
    free(lexer);
}

TokenType lexer_next(Lexer *lexer, Token *token) {
    token_destroy(token);
    lexer->context.token = token;
    int type = cobol_yylex(lexer->scanner);
    if (type == 0) {
        token->type = TOK_EOF;
        token->line = lexer->source.length == 0 ? 1 :
                      lexer->source.locations[lexer->source.length - 1].line;
        token->column = lexer->source.length == 0 ? 1 :
                        lexer->source.locations[lexer->source.length - 1].column;
    }
    return token->type;
}
