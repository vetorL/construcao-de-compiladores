#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>

static void print_escaped(FILE *stream, const char *text) {
    fputc('"', stream);
    for (const unsigned char *cursor = (const unsigned char *)text; *cursor != '\0'; cursor++) {
        switch (*cursor) {
        case '\\': fputs("\\\\", stream); break;
        case '"': fputs("\\\"", stream); break;
        case '\n': fputs("\\n", stream); break;
        case '\r': fputs("\\r", stream); break;
        case '\t': fputs("\\t", stream); break;
        default: fputc(*cursor, stream); break;
        }
    }
    fputc('"', stream);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILE.cob\n", argv[0]);
        return 2;
    }
    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror(argv[1]);
        return 2;
    }

    char *error_message = NULL;
    Lexer *lexer = lexer_create(input, &error_message);
    fclose(input);
    if (lexer == NULL) {
        fprintf(stderr, "cobol-lexer: %s\n",
                error_message == NULL ? "could not initialize lexer" : error_message);
        free(error_message);
        return 2;
    }

    size_t error_count = 0;
    Token token;
    token_init(&token);
    while (lexer_next(lexer, &token) != TOK_EOF) {
        if (token.type == TOK_ERROR) {
            fprintf(stderr, "Lexical error [%zu:%zu]: %s near ", token.line, token.column,
                    token.message == NULL ? "invalid token" : token.message);
            print_escaped(stderr, token.lexeme == NULL ? "" : token.lexeme);
            fputc('\n', stderr);
            error_count++;
        } else {
            printf("%zu:%zu\t%-16s ", token.line, token.column,
                   token_type_name(token.type));
            print_escaped(stdout, token.lexeme);
            putchar('\n');
        }
    }
    token_destroy(&token);
    lexer_destroy(lexer);

    if (error_count != 0) {
        fprintf(stderr, "\n%zu lexical error%s found.\n", error_count,
                error_count == 1 ? "" : "s");
        return 1;
    }
    return 0;
}
