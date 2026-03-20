/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#ifndef PARSER_HELPERS_H
#define PARSER_HELPERS_H

#include "parserDef.h"

typedef TokData (*SynNextFn)(DualBuf *db, ReservedMap *rm, int *errs);

void syn_pop_free(SymStack *ss);
void syn_drain(SymStack *ss);
void syn_skip_to_semi(TokData *cur, DualBuf *db, ReservedMap *rm, int *errs,
                      SynNextFn nfn);
int syn_is_decl_boundary(TokKind k);
int syn_is_block_boundary(TokKind k);
void syn_unwind_to_boundary(SymStack *ss, int (*pred)(TokKind), int eat_semi);

#endif
