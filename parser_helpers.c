/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#include "parser_helpers.h"
#include "parser.h"
#include <stdlib.h>

void syn_pop_free(SymStack *ss) {
  SymFrame *f = sstack_pop(ss);
  if (f)
    free(f);
}

void syn_drain(SymStack *ss) {
  while (!sstack_empty(ss))
    syn_pop_free(ss);
}

void syn_skip_to_semi(TokData *cur, DualBuf *db, ReservedMap *rm, int *errs,
                      SynNextFn nfn) {
  while (cur->kind != TK_EOF && cur->kind != TK_SEM)
    *cur = nfn(db, rm, errs);
  if (cur->kind == TK_SEM)
    *cur = nfn(db, rm, errs);
}

int syn_is_decl_boundary(TokKind k) {
  return (k == TK_SEM || k == TK_END || k == TK_MAIN || k == TK_EOF);
}

int syn_is_block_boundary(TokKind k) {
  return (k == TK_END || k == TK_ENDIF || k == TK_ENDWHILE || k == TK_MAIN ||
          k == TK_EOF);
}

void syn_unwind_to_boundary(SymStack *ss, int (*pred)(TokKind), int eat_semi) {
  while (!sstack_empty(ss)) {
    SymFrame *f = sstack_top(ss);
    if (f->sym.isTerm && pred((TokKind)f->sym.idx)) {
      if (eat_semi && f->sym.idx == TK_SEM)
        syn_pop_free(ss);
      break;
    }
    syn_pop_free(ss);
  }
}
