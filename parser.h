/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "parserDef.h"

RuleSet ruleset_load(const char *path);
FFSets ff_compute(RuleSet *rs);
void ptable_build(FFSets *ff, PredTable *pt, RuleSet *rs);
void ptable_show(PredTable *pt);

void sstack_init(SymStack *ss);
void sstack_push(SymStack *ss, Symbol sym, PNode *ref);
SymFrame *sstack_pop(SymStack *ss);
int sstack_empty(SymStack *ss);
SymFrame *sstack_top(SymStack *ss);
void sstack_free(SymStack *ss);

PNode *pnode_leaf(TokData td, PNode *par);
PNode *pnode_inner(NTerm nt, PNode *par);
void pnode_add_child(PNode *par, PNode *ch);

PNode *parse_source(char *src_path, PredTable *pt, RuleSet *rs, ReservedMap *rm,
                    int *err_out);
void ptree_dump(PNode *root, char *dst);
void ptree_free(PNode *root);

#endif
