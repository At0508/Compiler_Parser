/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#ifndef LEXER_HELPERS_H
#define LEXER_HELPERS_H

#include "lexerDef.h"

void lex_clear_tok(TokData *td, DualBuf *db);
void lex_push_char(TokData *td, int *p, char c);
void lex_maybe_unget(DualBuf *db, char c);
int lex_single_sym(TokData *td, char c);

#endif
