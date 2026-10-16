#define _POSIX_C_SOURCE 200809L

#include "fixed_format.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define INVALID_CONTINUATION ((char)0x1f)

static char *copy_string(const char *text) {
    size_t length = strlen(text) + 1;
    char *copy = malloc(length);
    if (copy != NULL) {
        memcpy(copy, text, length);
    }
    return copy;
}

static int reserve(SourceBuffer *buffer, size_t extra) {
    if (buffer->length + extra <= buffer->capacity) {
        return 1;
    }
    size_t capacity = buffer->capacity == 0 ? 256 : buffer->capacity;
    while (capacity < buffer->length + extra) {
        if (capacity > (size_t)-1 / 2) {
            return 0;
        }
        capacity *= 2;
    }
    char *new_text = realloc(buffer->text, capacity + 1);
    if (new_text == NULL) {
        return 0;
    }
    buffer->text = new_text;
    SourceLocation *new_locations = realloc(buffer->locations,
                                             capacity * sizeof(*new_locations));
    if (new_locations == NULL) {
        return 0;
    }
    buffer->locations = new_locations;
    buffer->capacity = capacity;
    return 1;
}

static int append_char(SourceBuffer *buffer, char value, size_t line, size_t column) {
    if (!reserve(buffer, 1)) {
        return 0;
    }
    buffer->text[buffer->length] = value;
    buffer->locations[buffer->length].line = line;
    buffer->locations[buffer->length].column = column;
    buffer->length++;
    buffer->text[buffer->length] = '\0';
    return 1;
}

static char update_quote(char quote, const char *line, size_t begin, size_t end) {
    for (size_t i = begin; i < end; i++) {
        if (quote == '\0' && (line[i] == '\'' || line[i] == '"')) {
            quote = line[i];
        } else if (quote != '\0' && line[i] == quote) {
            quote = '\0';
        }
    }
    return quote;
}

void source_buffer_init(SourceBuffer *buffer) {
    buffer->text = NULL;
    buffer->locations = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
}

void source_buffer_destroy(SourceBuffer *buffer) {
    free(buffer->text);
    free(buffer->locations);
    source_buffer_init(buffer);
}

int fixed_format_read(FILE *input, SourceBuffer *buffer, char **error_message) {
    char *line = NULL;
    size_t line_capacity = 0;
    size_t physical_line = 0;
    ssize_t read_count;
    char pending_quote = '\0';
    size_t pending_line = 0;

    *error_message = NULL;
    while ((read_count = getline(&line, &line_capacity, input)) >= 0) {
        physical_line++;
        size_t length = (size_t)read_count;
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
            length--;
        }

        char indicator = length >= 7 ? line[6] : ' ';
        if (indicator == '*' || indicator == '/' || indicator == 'D' || indicator == 'd') {
            continue;
        }

        size_t begin = length < 7 ? length : 7;
        size_t end = length < 72 ? length : 72;
        while (end > begin && (line[end - 1] == ' ' || line[end - 1] == '\t')) {
            end--;
        }

        if (indicator == '-') {
            if (pending_quote == '\0') {
                /* A continuation may split a word or follow a closed literal.
                   Retain its leading spaces: they separate complete words. */
                if (buffer->length > 1 && buffer->text[buffer->length - 1] == '\n' &&
                    buffer->text[buffer->length - 2] != '.' &&
                    buffer->text[buffer->length - 2] != '\n') {
                    buffer->length--;
                } else {
                    if (!append_char(buffer, INVALID_CONTINUATION, physical_line, 7)) {
                        goto memory_error;
                    }
                }
            } else {
                while (begin < end && isspace((unsigned char)line[begin])) {
                    begin++;
                }
                if (begin < end && line[begin] == pending_quote) {
                    begin++;
                }
            }
        } else if (pending_quote != '\0') {
            if (!append_char(buffer, '\n', pending_line, 73)) {
                goto memory_error;
            }
            pending_quote = '\0';
        }

        char initial_quote = pending_quote;
        for (size_t i = begin; i < end; i++) {
            if (!append_char(buffer, line[i], physical_line, i + 1)) {
                goto memory_error;
            }
        }
        pending_quote = update_quote(initial_quote, line, begin, end);
        pending_line = physical_line;
        if (pending_quote == '\0') {
            if (!append_char(buffer, '\n', physical_line, end + 1)) {
                goto memory_error;
            }
        }
    }

    free(line);
    if (ferror(input)) {
        *error_message = copy_string("could not read the source file");
        return 0;
    }
    if (pending_quote != '\0' && !append_char(buffer, '\n', pending_line, 73)) {
        goto memory_error_without_line;
    }
    if (buffer->text == NULL && !reserve(buffer, 1)) {
        goto memory_error_without_line;
    }
    buffer->text[buffer->length] = '\0';
    return 1;

memory_error:
    free(line);
memory_error_without_line:
    *error_message = copy_string("out of memory while reading the source file");
    return 0;
}
