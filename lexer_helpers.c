/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#include "lexer_helpers.h"
#include "lexer.h"
#include <string.h>

void lex_clear_tok(TokData *td, DualBuf *db) {
  memset(td, 0, sizeof(*td));
  td->ln = db->line;
}

void lex_push_char(TokData *td, int *p, char c) {
  if (*p < MAX_LEXEME - 1) {
    td->text[*p] = c;
    (*p)++;
  }
}

void lex_maybe_unget(DualBuf *db, char c) {
  if (c != '\0')
    dbuf_ungetc(db, 1);
}

int lex_single_sym(TokData *td, char c) {
  TokKind k;
  switch (c) {
  case '+':
    k = TK_PLUS;
    break;
  case '-':
    k = TK_MINUS;
    break;
  case '*':
    k = TK_MUL;
    break;
  case '/':
    k = TK_DIV;
    break;
  case '~':
    k = TK_NOT;
    break;
  case '(':
    k = TK_OP;
    break;
  case ')':
    k = TK_CL;
    break;
  case '[':
    k = TK_SQL;
    break;
  case ']':
    k = TK_SQR;
    break;
  case ',':
    k = TK_COMMA;
    break;
  case ';':
    k = TK_SEM;
    break;
  case ':':
    k = TK_COLON;
    break;
  case '.':
    k = TK_DOT;
    break;
  default:
    return 0;
  }
  td->kind = k;
  td->text[0] = c;
  td->text[1] = '\0';
  return 1;
}
