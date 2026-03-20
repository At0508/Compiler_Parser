# Group 50
# Samyak Savi          2022B3A70635P
# Chavi Gupta          2022B3A70637P
# Saumya Agarwal       2022B5A70943P
# Atharva Agrawal      2022B3A70597P
# Nishit Patel         2022B3A70568P
# Agrim Goyal          2022B3A71269P

CC       = gcc
CFLAGS   = -Wall -Wextra -g -std=c11
TARGET   = stage1exe

SRCS     = driver.c lexer.c parser.c lexer_helpers.c parser_helpers.c
OBJS     = driver.o lexer.o parser.o lexer_helpers.o parser_helpers.o

all: $(TARGET)

make: all

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lm

driver.o: driver.c lexer.h parser.h lexerDef.h parserDef.h
	$(CC) $(CFLAGS) -c driver.c

lexer.o: lexer.c lexerDef.h lexer_helpers.h
	$(CC) $(CFLAGS) -c lexer.c

lexer_helpers.o: lexer_helpers.c lexer_helpers.h lexerDef.h lexer.h
	$(CC) $(CFLAGS) -c lexer_helpers.c

parser.o: parser.c parserDef.h lexerDef.h lexer.h parser_helpers.h
	$(CC) $(CFLAGS) -c parser.c

parser_helpers.o: parser_helpers.c parser_helpers.h parserDef.h parser.h
	$(CC) $(CFLAGS) -c parser_helpers.c

clean:
	rm -f $(OBJS) $(TARGET)

cleanall:
	rm -f $(OBJS) $(TARGET) commentFreeCode.txt

help:
	@echo "Targets:"
	@echo "  all       - Build stage1exe (default)"
	@echo "  make      - Alias of 'all' (supports: make clean make)"
	@echo "  clean     - Remove object files and executable"
	@echo "  cleanall  - Remove all generated files"
	@echo "  help      - Show this message"

.PHONY: all make clean cleanall help
