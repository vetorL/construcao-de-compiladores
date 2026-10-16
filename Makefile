CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Isrc
FLEX ?= flex

.SUFFIXES:

TARGET := cobol-lexer
SOURCES := src/main.c src/lexer.c src/token.c src/fixed_format.c src/lexer.yy.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean test regen report

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(OBJECTS:.o=.d)

regen:
	$(FLEX) -o src/lexer.yy.c src/lexer.l

test: $(TARGET)
	sh tests/run_tests.sh
	python3 tests/test_extended.py

report: $(TARGET)
	python3 scripts/report.py

clean:
	$(RM) $(TARGET) $(OBJECTS) $(OBJECTS:.o=.d)
