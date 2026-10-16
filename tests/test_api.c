#include "lexer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static Lexer *from_text(const char *source) {
    FILE *file = tmpfile();
    assert(file != NULL);
    assert(fputs(source, file) >= 0);
    rewind(file);
    char *error = NULL;
    Lexer *lexer = lexer_create(file, &error);
    assert(lexer != NULL && error == NULL);
    fclose(file);
    free(error);
    return lexer;
}

int main(void) {
    Lexer *first = from_text("       PIC 9(2).\n");
    Lexer *second = from_text("       MOVE ACCOUNT TO TARGET.\n");
    Token one, two;
    token_init(&one);
    token_init(&two);
    assert(lexer_next(first, &one) == TOK_PIC);
    assert(lexer_next(second, &two) == TOK_MOVE);
    assert(lexer_next(first, &one) == TOK_PICTURE_STRING);
    assert(strcmp(one.lexeme, "9(2)") == 0);
    assert(one.line == 1 && one.column == 12);
    assert(lexer_next(second, &two) == TOK_IDENTIFIER);
    assert(strcmp(two.lexeme, "ACCOUNT") == 0);
    assert(lexer_next(first, &one) == TOK_PERIOD);
    assert(lexer_next(first, &one) == TOK_EOF);
    assert(lexer_next(first, &one) == TOK_EOF);
    token_destroy(&one);
    token_destroy(&two);
    lexer_destroy(first);
    lexer_destroy(second);
    return 0;
}
