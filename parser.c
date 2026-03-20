/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#include "lexer.h"
#include "lexerDef.h"
#include "parserDef.h"
#include "parser_helpers.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int resolve_terminal(const char *s) {
  for (int i = 0; i < NUM_TOKENS; i++)
    if (strcmp(s, tok_repr[i]) == 0)
      return i;
  return -1;
}

static int resolve_nonterminal(const char *s) {
  char buf[128];
  int j = 0;
  for (int i = 0; s[i]; i++)
    if (s[i] != '<' && s[i] != '>')
      buf[j++] = s[i];
  buf[j] = '\0';
  for (int i = 0; i < NUM_NONTERMINALS; i++)
    if (strcmp(buf, nt_repr[i]) == 0)
      return i;
  return -1;
}

static int is_overlong_varid(const char *lex) {
  size_t n = strlen(lex);
  if (n <= MAX_VARID_LEN)
    return 0;
  if (!(lex[0] >= 'b' && lex[0] <= 'd'))
    return 0;
  for (size_t i = 0; i < n; i++)
    if (!isalnum((unsigned char)lex[i]))
      return 0;
  return 1;
}

static int is_lone_unknown_sym(const char *lex) {
  if (strlen(lex) != 1)
    return 0;
  unsigned char c = (unsigned char)lex[0];
  return !(isalnum(c) || c == '_' || c == '#');
}

static void emit_lex_err(const TokData *td) {
  if (is_overlong_varid(td->text)) {
    fprintf(stderr,
            "Line %d \tError: Variable Identifier is longer than the "
            "prescribed length of 20 characters.\n",
            td->ln);
    return;
  }
  if (is_lone_unknown_sym(td->text)) {
    fprintf(stderr, "Line %d Error: Unknown Symbol <%s>\n", td->ln, td->text);
    return;
  }
  fprintf(stderr, "Line %d Error: Unknown pattern <%s>\n", td->ln, td->text);
}

static void emit_mismatch(const TokData *cur, TokKind exp) {
  fprintf(stderr,
          "Line %d  Error: The token %s for lexeme %s  does not match with the "
          "expected token %s\n",
          cur->ln, tok_repr[cur->kind], cur->text, tok_repr[exp]);
}

static void emit_invalid_top(const TokData *cur, NTerm nt) {
  fprintf(stderr,
          "Line %d Error: Invalid token %s encountered with value %s stack top "
          "%s\n",
          cur->ln, tok_repr[cur->kind], cur->text, nt_repr[nt]);
}

static int g_lex_err_line = -1;
static int g_syn_on_lex_line = 0;
static char g_lex_err_text[MAX_LEXEME] = "";

#define COND_DEPTH 64
typedef struct {
  int in_then;
  int in_else;
  int then_damaged;
  int then_line;
} CondState;
static CondState g_cond[COND_DEPTH];
static int g_ctop = -1;

static void cond_reset(void) {
  g_ctop = -1;
  memset(g_cond, 0, sizeof(g_cond));
}

static void cond_on_match(TokKind k, int ln) {
  if (k == TK_IF) {
    if (g_ctop + 1 < COND_DEPTH) {
      g_ctop++;
      g_cond[g_ctop].in_then = 0;
      g_cond[g_ctop].in_else = 0;
      g_cond[g_ctop].then_damaged = 0;
      g_cond[g_ctop].then_line = -1;
    }
    return;
  }
  if (g_ctop < 0)
    return;
  switch (k) {
  case TK_THEN:
    g_cond[g_ctop].in_then = 1;
    g_cond[g_ctop].in_else = 0;
    g_cond[g_ctop].then_line = -1;
    break;
  case TK_ELSE:
    g_cond[g_ctop].in_else = 1;
    break;
  case TK_ENDIF:
    g_ctop--;
    break;
  default:
    if (g_cond[g_ctop].in_then && !g_cond[g_ctop].in_else)
      g_cond[g_ctop].then_line = ln;
    break;
  }
}

static void cond_mark_damage(void) {
  if (g_ctop < 0)
    return;
  if (g_cond[g_ctop].in_then && !g_cond[g_ctop].in_else)
    g_cond[g_ctop].then_damaged = 1;
}

static int cond_then_bad(void) {
  return (g_ctop >= 0) ? g_cond[g_ctop].then_damaged : 0;
}

static int cond_then_line(void) {
  return (g_ctop >= 0) ? g_cond[g_ctop].then_line : -1;
}

static void cond_close(void) {
  if (g_ctop >= 0)
    g_ctop--;
}

static int dedup_check(const TokData *cur, int src, int *eline, TokKind *etag,
                       int *esrc, char ebuf[MAX_LEXEME]) {
  if (*eline == cur->ln && *etag == cur->kind && *esrc == src &&
      strcmp(ebuf, cur->text) == 0)
    return 0;
  *eline = cur->ln;
  *etag = cur->kind;
  *esrc = src;
  strcpy(ebuf, cur->text);
  return 1;
}

static int allow_syn_report(const TokData *cur, int src, int *eline,
                            TokKind *etag, int *esrc, char ebuf[MAX_LEXEME]) {
  if (!dedup_check(cur, src, eline, etag, esrc, ebuf))
    return 0;
  return 1;
}

static TokData next_significant(DualBuf *db, ReservedMap *rm, int *errs) {
  TokData td = scan_next(db, rm);
  while (td.kind == TK_COMMENT || td.kind == TK_ERROR) {
    if (td.kind == TK_ERROR) {
      if (td.ln != g_lex_err_line) {
        g_lex_err_line = td.ln;
        g_syn_on_lex_line = 0;
      }
      strncpy(g_lex_err_text, td.text, MAX_LEXEME - 1);
      g_lex_err_text[MAX_LEXEME - 1] = '\0';
      emit_lex_err(&td);
      (*errs)++;
      cond_mark_damage();
    }
    td = scan_next(db, rm);
  }
  return td;
}

static int is_panic_anchor(TokKind k) {
  return (k == TK_SEM || k == TK_ENDRECORD || k == TK_ENDUNION ||
          k == TK_ENDIF || k == TK_ENDWHILE || k == TK_ELSE || k == TK_CL ||
          k == TK_SQR || k == TK_END || k == TK_MAIN || k == TK_EOF);
}

static int is_structural_terminal(TokKind k) {
  return (k == TK_END || k == TK_MAIN);
}

static int is_expr_level_nt(int ni) {
  return (ni == NT_var || ni == NT_A || ni == NT_factor || ni == NT_term ||
          ni == NT_termPrime || ni == NT_arithmeticExpression ||
          ni == NT_expPrime || ni == NT_booleanExpression ||
          ni == NT_logicalOp ||
          ni == NT_relationalOp || ni == NT_highPrecedenceOperators ||
          ni == NT_lowPrecedenceOperators ||
          ni == NT_option_single_constructed);
}

static int suppress_recovery_msg(const TokData *cur, int ni) {
  int dangling = (cur->kind == TK_ELSE && g_ctop >= 0 &&
                  g_cond[g_ctop].in_then && !g_cond[g_ctop].in_else &&
                  (ni == NT_option_single_constructed || ni == NT_termPrime ||
                   ni == NT_expPrime || ni == NT_arithmeticExpression));
  int lex_noise =
      (cur->ln == g_lex_err_line && g_lex_err_text[0] == '<' &&
       (ni == NT_option_single_constructed || ni == NT_arithmeticExpression ||
        ni == NT_termPrime || ni == NT_expPrime));
  int trailing_num_after_lex =
      (ni == NT_option_single_constructed && cur->kind == TK_NUM &&
       g_lex_err_text[0] == '<' && cur->ln >= g_lex_err_line);
  return dangling || lex_noise || trailing_num_after_lex;
}

static void maybe_report_invalid(const TokData *cur, int ni, int *errs,
                                 int *eline, TokKind *etag, int *esrc,
                                 char ebuf[MAX_LEXEME]) {
  if (suppress_recovery_msg(cur, ni))
    return;
  if (allow_syn_report(cur, 1, eline, etag, esrc, ebuf)) {
    emit_invalid_top(cur, (NTerm)ni);
    (*errs)++;
  }
}

static int first_of_sym(RuleSet *rs __attribute__((unused)), FFSets *ff,
                        Symbol s, int out[]) {
  if (s.idx == SYM_EPS)
    return 1;
  if (s.isTerm) {
    out[s.idx] = 1;
    return 0;
  }
  int n = s.idx;
  for (int t = 0; t < NUM_TOKENS; t++)
    if (ff->fst[n][t])
      out[t] = 1;
  return ff->nullable[n];
}

RuleSet ruleset_load(const char *path) {
  RuleSet rs;
  rs.nprods = 0;
  FILE *fp = fopen(path, "r");
  if (!fp) {
    fprintf(stderr, "Error: cannot open grammar file '%s'\n", path);
    return rs;
  }

  char line[512];
  while (fgets(line, sizeof(line), fp)) {
    int len = (int)strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
      line[--len] = '\0';
    if (len == 0)
      continue;

    char *lhs_s = strtok(line, " \t");
    if (!lhs_s)
      continue;
    char *arrow = strtok(NULL, " \t");
    if (!arrow)
      continue;
    int ni = resolve_nonterminal(lhs_s);
    if (ni < 0) {
      fprintf(stderr, "Warning: unknown NT '%s', skipping rule\n", lhs_s);
      continue;
    }

    Production *p = &rs.prods[rs.nprods];
    p->head = (NTerm)ni;
    p->bodyLen = 0;

    char *w;
    while ((w = strtok(NULL, " \t")) != NULL) {
      if (strcmp(w, "eps") == 0) {
        p->body[p->bodyLen].isTerm = 0;
        p->body[p->bodyLen].idx = SYM_EPS;
      } else if (w[0] == '<') {
        int x = resolve_nonterminal(w);
        if (x < 0) {
          fprintf(stderr, "Warning: unknown NT in RHS '%s'\n", w);
          continue;
        }
        p->body[p->bodyLen].isTerm = 0;
        p->body[p->bodyLen].idx = x;
      } else {
        int x = resolve_terminal(w);
        if (x < 0) {
          fprintf(stderr, "Warning: unknown terminal '%s'\n", w);
          continue;
        }
        p->body[p->bodyLen].isTerm = 1;
        p->body[p->bodyLen].idx = x;
      }
      p->bodyLen++;
    }
    rs.nprods++;
    if (rs.nprods >= RULES_CAP)
      break;
  }
  fclose(fp);
  return rs;
}

FFSets ff_compute(RuleSet *rs) {
  FFSets ff;
  memset(&ff, 0, sizeof(ff));

  int changed = 1;
  while (changed) {
    changed = 0;
    for (int r = 0; r < rs->nprods; r++) {
      Production *p = &rs->prods[r];
      int h = p->head;
      if (p->bodyLen == 1 && p->body[0].idx == SYM_EPS) {
        if (!ff.nullable[h]) {
          ff.nullable[h] = 1;
          changed = 1;
        }
        continue;
      }
      int all_null = 1;
      for (int i = 0; i < p->bodyLen; i++) {
        Symbol s = p->body[i];
        if (s.idx == SYM_EPS)
          continue;
        if (s.isTerm) {
          if (!ff.fst[h][s.idx]) {
            ff.fst[h][s.idx] = 1;
            changed = 1;
          }
          all_null = 0;
          break;
        } else {
          for (int t = 0; t < NUM_TOKENS; t++)
            if (ff.fst[s.idx][t] && !ff.fst[h][t]) {
              ff.fst[h][t] = 1;
              changed = 1;
            }
          if (!ff.nullable[s.idx]) {
            all_null = 0;
            break;
          }
        }
      }
      if (all_null && !ff.nullable[h]) {
        ff.nullable[h] = 1;
        changed = 1;
      }
    }
  }

  ff.flw[NT_program][TK_EOF] = 1;
  changed = 1;
  while (changed) {
    changed = 0;
    for (int r = 0; r < rs->nprods; r++) {
      Production *p = &rs->prods[r];
      int h = p->head;
      for (int i = 0; i < p->bodyLen; i++) {
        Symbol s = p->body[i];
        if (s.isTerm || s.idx == SYM_EPS)
          continue;
        int b = s.idx;
        int beta_null = 1;
        for (int j = i + 1; j < p->bodyLen; j++) {
          Symbol bs = p->body[j];
          if (bs.idx == SYM_EPS)
            continue;
          if (bs.isTerm) {
            if (!ff.flw[b][bs.idx]) {
              ff.flw[b][bs.idx] = 1;
              changed = 1;
            }
            beta_null = 0;
            break;
          } else {
            for (int t = 0; t < NUM_TOKENS; t++)
              if (ff.fst[bs.idx][t] && !ff.flw[b][t]) {
                ff.flw[b][t] = 1;
                changed = 1;
              }
            if (!ff.nullable[bs.idx]) {
              beta_null = 0;
              break;
            }
          }
        }
        if (beta_null) {
          for (int t = 0; t < NUM_TOKENS; t++)
            if (ff.flw[h][t] && !ff.flw[b][t]) {
              ff.flw[b][t] = 1;
              changed = 1;
            }
        }
      }
    }
  }
  return ff;
}

void ptable_build(FFSets *ff, PredTable *pt, RuleSet *rs) {
  for (int i = 0; i < NUM_NONTERMINALS; i++)
    for (int j = 0; j < NUM_TOKENS; j++)
      pt->entry[i][j] = CELL_EMPTY;

  for (int r = 0; r < rs->nprods; r++) {
    Production *p = &rs->prods[r];
    int h = p->head;
    int rhs_fst[NUM_TOKENS];
    memset(rhs_fst, 0, sizeof(rhs_fst));
    int eps_ok = 1;

    if (p->bodyLen == 1 && p->body[0].idx == SYM_EPS) {
      eps_ok = 1;
    } else {
      eps_ok = 1;
      for (int i = 0; i < p->bodyLen; i++) {
        Symbol s = p->body[i];
        if (s.idx == SYM_EPS)
          continue;
        int can = first_of_sym(rs, ff, s, rhs_fst);
        if (!can) {
          eps_ok = 0;
          break;
        }
      }
    }
    for (int t = 0; t < NUM_TOKENS; t++)
      if (rhs_fst[t] &&
          (pt->entry[h][t] == CELL_EMPTY || pt->entry[h][t] == CELL_SYNC))
        pt->entry[h][t] = r;

    if (eps_ok) {
      for (int t = 0; t < NUM_TOKENS; t++)
        if (ff->flw[h][t] && pt->entry[h][t] == CELL_EMPTY)
          pt->entry[h][t] = r;
    }
  }
  for (int n = 0; n < NUM_NONTERMINALS; n++)
    for (int t = 0; t < NUM_TOKENS; t++)
      if (pt->entry[n][t] == CELL_EMPTY && ff->flw[n][t])
        pt->entry[n][t] = CELL_SYNC;
}

void ptable_show(PredTable *pt) {
  printf("\n===== PARSE TABLE =====\n");
  printf("%-30s", "");
  for (int t = 0; t < NUM_TOKENS; t++) {
    if (t == TK_COMMENT || t == TK_ERROR)
      continue;
    printf("%-14s", tok_repr[t]);
  }
  printf("\n");
  for (int n = 0; n < NUM_NONTERMINALS; n++) {
    printf("%-30s", nt_repr[n]);
    for (int t = 0; t < NUM_TOKENS; t++) {
      if (t == TK_COMMENT || t == TK_ERROR)
        continue;
      int v = pt->entry[n][t];
      if (v == CELL_EMPTY)
        printf("%-14s", "---");
      else if (v == CELL_SYNC)
        printf("%-14s", "SYNCH");
      else
        printf("R%-13d", v);
    }
    printf("\n");
  }
  printf("========================\n");
}

void sstack_init(SymStack *ss) { ss->head = NULL; }

void sstack_push(SymStack *ss, Symbol sym, PNode *ref) {
  SymFrame *f = (SymFrame *)malloc(sizeof(SymFrame));
  f->sym = sym;
  f->node = ref;
  f->prev = ss->head;
  ss->head = f;
}

SymFrame *sstack_pop(SymStack *ss) {
  if (!ss->head)
    return NULL;
  SymFrame *f = ss->head;
  ss->head = f->prev;
  return f;
}

int sstack_empty(SymStack *ss) { return ss->head == NULL; }
SymFrame *sstack_top(SymStack *ss) { return ss->head; }

void sstack_free(SymStack *ss) {
  while (ss->head) {
    SymFrame *t = ss->head;
    ss->head = t->prev;
    free(t);
  }
}

PNode *pnode_leaf(TokData td, PNode *par) {
  PNode *n = (PNode *)calloc(1, sizeof(PNode));
  n->leaf = 1;
  n->tok = td;
  n->up = par;
  return n;
}

PNode *pnode_inner(NTerm nt, PNode *par) {
  PNode *n = (PNode *)calloc(1, sizeof(PNode));
  n->leaf = 0;
  n->nt = nt;
  n->up = par;
  n->prodIdx = -1;
  return n;
}

void pnode_add_child(PNode *par, PNode *ch) {
  ch->up = par;
  if (!par->child) {
    par->child = ch;
    return;
  }
  PNode *s = par->child;
  while (s->sib)
    s = s->sib;
  s->sib = ch;
}

PNode *parse_source(char *src_path, PredTable *pt, RuleSet *rs, ReservedMap *rm,
                    int *err_out) {
  FILE *sfp = fopen(src_path, "r");
  if (!sfp) {
    fprintf(stderr, "Error: cannot open source file '%s'\n", src_path);
    return NULL;
  }

  DualBuf db;
  dbuf_open(&db, sfp);

  PNode *root = pnode_inner(NT_program, NULL);

  SymStack ss;
  sstack_init(&ss);

  Symbol dollar;
  dollar.isTerm = 1;
  dollar.idx = TK_EOF;
  sstack_push(&ss, dollar, NULL);
  Symbol start;
  start.isTerm = 0;
  start.idx = NT_program;
  sstack_push(&ss, start, root);

  int nerr = 0;
  g_lex_err_line = -1;
  g_syn_on_lex_line = 0;
  g_lex_err_text[0] = '\0';
  cond_reset();
  int el = -1;
  TokKind et = TK_EOF;
  int esrc = -1;
  char ebuf[MAX_LEXEME];
  ebuf[0] = '\0';
  TokData cur = next_significant(&db, rm, &nerr);

  while (!sstack_empty(&ss)) {
    SymFrame *top = sstack_top(&ss);
    Symbol ts = top->sym;
    PNode *tn = top->node;

    if (ts.isTerm) {
      if (ts.idx == TK_EOF && cur.kind != TK_EOF) {
        cur = next_significant(&db, rm, &nerr);
        continue;
      }
      if (ts.idx == (int)cur.kind) {
        syn_pop_free(&ss);
        if (tn) {
          tn->leaf = 1;
          tn->tok = cur;
        }
        cond_on_match(cur.kind, cur.ln);
        if (cur.kind != TK_EOF)
          cur = next_significant(&db, rm, &nerr);
      } else {
        cond_mark_damage();

        if (is_structural_terminal((TokKind)ts.idx)) {
          if (allow_syn_report(&cur, 1, &el, &et, &esrc, ebuf)) {
            emit_mismatch(&cur, (TokKind)ts.idx);
            nerr++;
          }
          while (cur.kind != TK_EOF && (int)cur.kind != ts.idx)
            cur = next_significant(&db, rm, &nerr);
          continue;
        }

        int decl_colon = (ts.idx == TK_COLON && tn && tn->up &&
                          tn->up->nt == NT_declaration && cur.kind == TK_ID);

        if (decl_colon) {
          if (allow_syn_report(&cur, 1, &el, &et, &esrc, ebuf)) {
            emit_invalid_top(&cur, NT_otherStmts);
            nerr++;
          }
          syn_skip_to_semi(&cur, &db, rm, &nerr, next_significant);
          syn_unwind_to_boundary(&ss, syn_is_decl_boundary, 1);
          continue;
        }

        int suppress_assign =
            (cur.ln == g_lex_err_line && ts.idx == TK_ASSIGNOP &&
             g_lex_err_text[0] == '<');
        int suppress_damaged_then_sem =
            (cond_then_bad() && ts.idx == TK_SEM && cur.kind == TK_CL);
        if (!suppress_assign && !suppress_damaged_then_sem &&
            allow_syn_report(&cur, 0, &el, &et, &esrc, ebuf)) {
          emit_mismatch(&cur, (TokKind)ts.idx);
          nerr++;
        }
        if (tn) {
          tn->leaf = 1;
          tn->tok.kind = (TokKind)ts.idx;
          tn->tok.ln = cur.ln;
          strcpy(tn->tok.text, "----");
        }
        syn_pop_free(&ss);
      }
    } else if (ts.idx == SYM_EPS) {
      syn_pop_free(&ss);
    } else {
      int ni = ts.idx;
      int ri = pt->entry[ni][cur.kind];

      if (cond_then_bad() && g_ctop >= 0 && ni == NT_otherStmts &&
          cur.kind == TK_CL) {
        int base_ln = cond_then_line();
        if (base_ln <= 0)
          base_ln = cur.ln;

        TokData fake = cur;
        fake.ln = base_ln;

        if (allow_syn_report(&fake, 1, &el, &et, &esrc, ebuf)) {
          emit_invalid_top(&fake, NT_var);
          nerr++;
        }
        if (allow_syn_report(&fake, 0, &el, &et, &esrc, ebuf)) {
          emit_mismatch(&fake, TK_SEM);
          nerr++;
        }

        while (cur.kind != TK_EOF && cur.kind != TK_ENDIF)
          cur = next_significant(&db, rm, &nerr);
        if (cur.kind == TK_ENDIF) {
          if (allow_syn_report(&cur, 1, &el, &et, &esrc, ebuf)) {
            emit_invalid_top(&cur, NT_otherStmts);
            nerr++;
          }
          cur = next_significant(&db, rm, &nerr);
        }

        cond_close();
        syn_unwind_to_boundary(&ss, syn_is_block_boundary, 0);
        continue;
      }

      if (ni == NT_elsePart && cur.kind == TK_ELSE && cond_then_bad()) {
        syn_pop_free(&ss);
        while (cur.kind != TK_EOF && cur.kind != TK_ENDIF)
          cur = next_significant(&db, rm, &nerr);
        if (cur.kind == TK_ENDIF) {
          if (allow_syn_report(&cur, 1, &el, &et, &esrc, ebuf)) {
            emit_invalid_top(&cur, NT_otherStmts);
            nerr++;
          }
          cur = next_significant(&db, rm, &nerr);
        }
        cond_close();
        syn_unwind_to_boundary(&ss, syn_is_block_boundary, 0);
        continue;
      }

      if (ni == NT_elsePart && cur.kind == TK_ENDIF && cond_then_bad()) {
        if (allow_syn_report(&cur, 1, &el, &et, &esrc, ebuf)) {
          emit_invalid_top(&cur, NT_otherStmts);
          nerr++;
        }
        syn_pop_free(&ss);
        cur = next_significant(&db, rm, &nerr);
        cond_close();
        syn_unwind_to_boundary(&ss, syn_is_block_boundary, 0);
        continue;
      }

      if (ri >= 0 && ri != CELL_SYNC) {
        syn_pop_free(&ss);
        if (tn)
          tn->prodIdx = ri;
        Production *p = &rs->prods[ri];

        if (p->bodyLen == 1 && p->body[0].idx == SYM_EPS) {
          /* epsilon */
        } else {
          PNode *kids[RHS_CAP];
          for (int i = 0; i < p->bodyLen; i++) {
            Symbol s = p->body[i];
            if (s.isTerm) {
              TokData dummy;
              memset(&dummy, 0, sizeof(dummy));
              dummy.kind = (TokKind)s.idx;
              dummy.ln = -1;
              strcpy(dummy.text, "----");
              kids[i] = pnode_leaf(dummy, tn);
            } else {
              kids[i] = pnode_inner((NTerm)s.idx, tn);
            }
            pnode_add_child(tn, kids[i]);
          }
          for (int i = p->bodyLen - 1; i >= 0; i--)
            sstack_push(&ss, p->body[i], kids[i]);
        }
      } else if (ri == CELL_SYNC) {
        cond_mark_damage();
        maybe_report_invalid(&cur, ni, &nerr, &el, &et, &esrc, ebuf);
        syn_pop_free(&ss);
      } else {
        cond_mark_damage();
        maybe_report_invalid(&cur, ni, &nerr, &el, &et, &esrc, ebuf);
        if (cur.kind == TK_EOF) {
          syn_drain(&ss);
          break;
        }

        if (is_expr_level_nt(ni) && cur.kind != TK_CL) {
          while (cur.kind != TK_EOF && cur.kind != TK_SEM &&
                 cur.kind != TK_CL && cur.kind != TK_THEN && cur.kind != TK_END)
            cur = next_significant(&db, rm, &nerr);
          while (!sstack_empty(&ss)) {
            SymFrame *f = sstack_top(&ss);
            if (f->sym.isTerm) {
              TokKind tk = (TokKind)f->sym.idx;
              if (tk == TK_END || tk == TK_MAIN || tk == TK_EOF || tk == TK_SEM)
                break;
            } else {
              int nt = f->sym.idx;
              if (nt == NT_otherStmts || nt == NT_elsePart || nt == NT_stmts ||
                  nt == NT_returnStmt)
                break;
            }
            syn_pop_free(&ss);
          }
          continue;
        }

        int nr = pt->entry[ni][cur.kind];
        int do_pop = 0;

        if (nr == CELL_EMPTY && is_panic_anchor(cur.kind)) {
          do_pop = 1;
        } else {
          while (nr == CELL_EMPTY) {
            cur = next_significant(&db, rm, &nerr);
            if (cur.kind == TK_EOF)
              break;
            nr = pt->entry[ni][cur.kind];
            if (nr == CELL_EMPTY && is_panic_anchor(cur.kind)) {
              do_pop = 1;
              break;
            }
          }
        }
        if (cur.kind == TK_EOF) {
          syn_drain(&ss);
          break;
        }
        if (do_pop)
          syn_pop_free(&ss);
      }
    }
  }

  fclose(sfp);
  *err_out = nerr;
  if (nerr == 0)
    printf("Input source code is syntactically correct...........\n");
  return root;
}

static void inorder_walk(PNode *nd, FILE *fp) {
  if (!nd)
    return;
  if (nd->leaf) {
    char par_name[64];
    if (nd->up)
      snprintf(par_name, sizeof(par_name), "%s", nt_repr[nd->up->nt]);
    else
      strcpy(par_name, "ROOT");

    char lex_buf[MAX_LEXEME];
    if (strlen(nd->tok.text) > 0)
      strcpy(lex_buf, nd->tok.text);
    else
      strcpy(lex_buf, "----");

    char val_buf[64];
    if (nd->tok.kind == TK_NUM)
      snprintf(val_buf, sizeof(val_buf), "%d", nd->tok.nv.ival);
    else if (nd->tok.kind == TK_RNUM)
      snprintf(val_buf, sizeof(val_buf), "%lf", nd->tok.nv.rval);
    else
      strcpy(val_buf, "----");

    fprintf(fp, "%-25s %-15s %-6d %-20s %-15s %-25s %-12s %-25s\n", lex_buf,
            "Leaf", nd->tok.ln, tok_repr[nd->tok.kind], val_buf, par_name,
            "yes", tok_repr[nd->tok.kind]);
    return;
  }

  if (nd->child)
    inorder_walk(nd->child, fp);

  char par_name[64];
  if (nd->up)
    snprintf(par_name, sizeof(par_name), "%s", nt_repr[nd->up->nt]);
  else
    strcpy(par_name, "ROOT");

  fprintf(fp, "%-25s %-15s %-6s %-20s %-15s %-25s %-12s %-25s\n", "----",
          "Non-Terminal", "----", "----", "----", par_name, "no",
          nt_repr[nd->nt]);

  if (nd->child) {
    PNode *s = nd->child->sib;
    while (s) {
      inorder_walk(s, fp);
      s = s->sib;
    }
  }
}

void ptree_dump(PNode *root, char *dst) {
  FILE *fp = fopen(dst, "w");
  if (!fp) {
    fprintf(stderr, "Error: cannot open output file '%s'\n", dst);
    return;
  }
  fprintf(fp, "%-25s %-15s %-6s %-20s %-15s %-25s %-12s %-25s\n", "lexeme",
          "CurrentNode", "lineno", "tokenName", "valueIfNumber",
          "parentNodeSymbol", "isLeafNode", "NodeSymbol");
  fprintf(fp, "-------------------------------------------"
              "-------------------------------------------"
              "-------------------------------------------\n");
  inorder_walk(root, fp);
  fclose(fp);
}

void ptree_free(PNode *root) {
  if (!root)
    return;
  ptree_free(root->child);
  ptree_free(root->sib);
  free(root);
}
