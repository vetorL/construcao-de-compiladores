#ifndef COBOL_FIXED_FORMAT_H
#define COBOL_FIXED_FORMAT_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    size_t line;
    size_t column;
} SourceLocation;

typedef struct {
    char *text;
    SourceLocation *locations;
    size_t length;
    size_t capacity;
} SourceBuffer;

void source_buffer_init(SourceBuffer *buffer);
void source_buffer_destroy(SourceBuffer *buffer);
int fixed_format_read(FILE *input, SourceBuffer *buffer, char **error_message);

#endif
