/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#ifndef LEXER_H
#define LEXER_H

#include "lexerDef.h"

void dbuf_open(DualBuf *db, FILE *fp);
FILE *getStream(FILE *fp);
char dbuf_getc(DualBuf *db);
void dbuf_ungetc(DualBuf *db, int n);

void rmap_init(ReservedMap *rm);
void rmap_destroy(ReservedMap *rm);
TokKind rmap_find(ReservedMap *rm, const char *w);

TokData scan_next(DualBuf *db, ReservedMap *rm);

void strip_comments(char *src_path, char *dst_path);

#endif
