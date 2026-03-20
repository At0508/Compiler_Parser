/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#include "lexerDef.h"
#include "lexer_helpers.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static unsigned int rmap_hash(const char *s) {
  unsigned int h = 2166136261u;
  while (*s) {
    h ^= (unsigned char)(*s);
    h *= 16777619u;
    s++;
  }
  return h % RMAP_BUCKETS;
}

void rmap_init(ReservedMap *rm) {
  memset(rm->slots, 0, sizeof(rm->slots));
  struct {
    const char *w;
    TokKind k;
  } kws[] = {{"with", TK_WITH},
             {"parameters", TK_PARAMETERS},
             {"end", TK_END},
             {"while", TK_WHILE},
             {"union", TK_UNION},
             {"endunion", TK_ENDUNION},
             {"definetype", TK_DEFINETYPE},
             {"as", TK_AS},
             {"type", TK_TYPE},
             {"global", TK_GLOBAL},
             {"parameter", TK_PARAMETER},
             {"list", TK_LIST},
             {"input", TK_INPUT},
             {"output", TK_OUTPUT},
             {"int", TK_INT},
             {"real", TK_REAL},
             {"endwhile", TK_ENDWHILE},
             {"if", TK_IF},
             {"then", TK_THEN},
             {"endif", TK_ENDIF},
             {"read", TK_READ},
             {"write", TK_WRITE},
             {"return", TK_RETURN},
             {"call", TK_CALL},
             {"record", TK_RECORD},
             {"endrecord", TK_ENDRECORD},
             {"else", TK_ELSE},
             {NULL, TK_EOF}};
  for (int i = 0; kws[i].w != NULL; i++) {
    unsigned int slot = rmap_hash(kws[i].w);
    ResEntry *e = (ResEntry *)malloc(sizeof(ResEntry));
    strcpy(e->name, kws[i].w);
    e->kind = kws[i].k;
    e->next = rm->slots[slot];
    rm->slots[slot] = e;
  }
}

void rmap_destroy(ReservedMap *rm) {
  for (int i = 0; i < RMAP_BUCKETS; i++) {
    ResEntry *cur = rm->slots[i];
    while (cur) {
      ResEntry *nxt = cur->next;
      free(cur);
      cur = nxt;
    }
    rm->slots[i] = NULL;
  }
}

TokKind rmap_find(ReservedMap *rm, const char *w) {
  unsigned int slot = rmap_hash(w);
  ResEntry *cur = rm->slots[slot];
  while (cur) {
    if (strcmp(cur->name, w) == 0)
      return cur->kind;
    cur = cur->next;
  }
  return TK_FIELDID;
}

void dbuf_open(DualBuf *db, FILE *fp) {
  db->fp = fp;
  db->activeSide = 0;
  db->pos = 0;
  db->line = 1;
  db->atEnd = 0;
  db->lenA = (int)fread(db->bufA, 1, DBUF_SIZE, fp);
  db->lenB = 0;
}

FILE *getStream(FILE *fp) { return fp; }

char dbuf_getc(DualBuf *db) {
  if (db->atEnd)
    return '\0';

  char *cur_buf = (db->activeSide == 0) ? db->bufA : db->bufB;
  int cur_len = (db->activeSide == 0) ? db->lenA : db->lenB;

  if (db->pos >= cur_len) {
    if (db->activeSide == 0) {
      db->lenB = (int)fread(db->bufB, 1, DBUF_SIZE, db->fp);
      if (db->lenB == 0) {
        db->atEnd = 1;
        return '\0';
      }
      db->activeSide = 1;
    } else {
      db->lenA = (int)fread(db->bufA, 1, DBUF_SIZE, db->fp);
      if (db->lenA == 0) {
        db->atEnd = 1;
        return '\0';
      }
      db->activeSide = 0;
    }
    db->pos = 0;
    cur_buf = (db->activeSide == 0) ? db->bufA : db->bufB;
  }

  char ch = cur_buf[db->pos];
  db->pos++;
  return ch;
}

void dbuf_ungetc(DualBuf *db, int n) {
  for (int i = 0; i < n; i++) {
    if (db->pos > 0) {
      db->pos--;
    } else {
      if (db->activeSide == 0 && db->lenB > 0) {
        db->activeSide = 1;
        db->pos = db->lenB - 1;
      } else if (db->activeSide == 1 && db->lenA > 0) {
        db->activeSide = 0;
        db->pos = db->lenA - 1;
      }
    }
  }
  db->atEnd = 0;
}

static int is_bd(char c) { return (c >= 'b' && c <= 'd'); }
static int is_27(char c) { return (c >= '2' && c <= '7'); }

TokData scan_next(DualBuf *db, ReservedMap *rm) {
  TokData td;
  lex_clear_tok(&td, db);

  char ch;
  int p = 0;

  for (;;) {
    ch = dbuf_getc(db);
    if (ch == '\0') {
      td.kind = TK_EOF;
      strcpy(td.text, "$");
      return td;
    }
    if (ch == '\n') {
      db->line++;
      continue;
    }
    if (ch == ' ' || ch == '\t' || ch == '\r')
      continue;
    break;
  }

  td.ln = db->line;

  switch (ch) {

  case '%':
    td.kind = TK_COMMENT;
    td.text[0] = '%';
    td.text[1] = '\0';
    for (;;) {
      ch = dbuf_getc(db);
      if (ch == '\n') {
        db->line++;
        break;
      }
      if (ch == '\0')
        break;
    }
    return td;

  case '+':
  case '*':
  case '/':
  case '~':
  case '(':
  case ')':
  case '[':
  case ']':
  case ',':
  case ';':
  case ':':
  case '.':
  case '-':
    if (lex_single_sym(&td, ch))
      return td;
    break;

  case '<':
    lex_push_char(&td, &p, '<');
    ch = dbuf_getc(db);
    if (ch == '=') {
      lex_push_char(&td, &p, '=');
      td.kind = TK_LE;
      return td;
    }
    if (ch == '-') {
      lex_push_char(&td, &p, '-');
      char c2 = dbuf_getc(db);
      if (c2 == '-') {
        lex_push_char(&td, &p, '-');
        char c3 = dbuf_getc(db);
        if (c3 == '-') {
          lex_push_char(&td, &p, '-');
          td.kind = TK_ASSIGNOP;
          return td;
        }
        lex_maybe_unget(db, c3);
        td.kind = TK_ERROR;
        return td;
      }
      lex_maybe_unget(db, c2);
      td.kind = TK_ERROR;
      return td;
    }
    lex_maybe_unget(db, ch);
    td.kind = TK_LT;
    return td;

  case '>':
    lex_push_char(&td, &p, '>');
    ch = dbuf_getc(db);
    if (ch == '=') {
      lex_push_char(&td, &p, '=');
      td.kind = TK_GE;
      return td;
    }
    lex_maybe_unget(db, ch);
    td.kind = TK_GT;
    return td;

  case '=':
    lex_push_char(&td, &p, '=');
    ch = dbuf_getc(db);
    if (ch == '=') {
      lex_push_char(&td, &p, '=');
      td.kind = TK_EQ;
      return td;
    }
    lex_maybe_unget(db, ch);
    td.kind = TK_ERROR;
    return td;

  case '!':
    lex_push_char(&td, &p, '!');
    ch = dbuf_getc(db);
    if (ch == '=') {
      lex_push_char(&td, &p, '=');
      td.kind = TK_NE;
      return td;
    }
    lex_maybe_unget(db, ch);
    td.kind = TK_ERROR;
    return td;

  case '&': {
    lex_push_char(&td, &p, '&');
    char c2 = dbuf_getc(db);
    char c3 = dbuf_getc(db);
    if (c2 == '&' && c3 == '&') {
      lex_push_char(&td, &p, '&');
      lex_push_char(&td, &p, '&');
      td.kind = TK_AND;
      return td;
    }
    lex_maybe_unget(db, c3);
    lex_maybe_unget(db, c2);
    if (c2 == '&') {
      lex_push_char(&td, &p, '&');
      dbuf_getc(db);
    }
    td.kind = TK_ERROR;
    return td;
  }

  case '@': {
    lex_push_char(&td, &p, '@');
    char c2 = dbuf_getc(db);
    char c3 = dbuf_getc(db);
    if (c2 == '@' && c3 == '@') {
      lex_push_char(&td, &p, '@');
      lex_push_char(&td, &p, '@');
      td.kind = TK_OR;
      return td;
    }
    lex_maybe_unget(db, c3);
    lex_maybe_unget(db, c2);
    if (c2 == '@') {
      lex_push_char(&td, &p, '@');
      dbuf_getc(db);
    }
    td.kind = TK_ERROR;
    return td;
  }

  case '#':
    lex_push_char(&td, &p, '#');
    ch = dbuf_getc(db);
    if (ch >= 'a' && ch <= 'z') {
      lex_push_char(&td, &p, ch);
      for (;;) {
        ch = dbuf_getc(db);
        if (ch >= 'a' && ch <= 'z')
          lex_push_char(&td, &p, ch);
        else {
          lex_maybe_unget(db, ch);
          break;
        }
      }
      td.kind = TK_RUID;
      return td;
    }
    lex_maybe_unget(db, ch);
    td.kind = TK_ERROR;
    return td;

  case '_':
    td.text[p++] = '_';
    ch = dbuf_getc(db);
    if (ch == 'm') {
      td.text[p++] = 'm';
      char rest[] = "ain";
      int ok = 1;
      for (int i = 0; i < 3; i++) {
        ch = dbuf_getc(db);
        if (ch == rest[i]) {
          td.text[p++] = ch;
        } else {
          ok = 0;
          if (ch != '\0') {
            if (isalpha(ch))
              td.text[p++] = ch;
            else
              dbuf_ungetc(db, 1);
          }
          break;
        }
      }
      if (ok && p == 5) {
        ch = dbuf_getc(db);
        if (!isalpha(ch) && !isdigit(ch)) {
          if (ch != '\0')
            dbuf_ungetc(db, 1);
          td.kind = TK_MAIN;
          return td;
        }
        if (isalpha(ch))
          td.text[p++] = ch;
        else if (isdigit(ch)) {
          td.text[p++] = ch;
          goto fnid_digits;
        }
      }
      for (;;) {
        ch = dbuf_getc(db);
        if (isalpha(ch)) {
          if (p < MAX_LEXEME - 1)
            td.text[p++] = ch;
        } else
          break;
      }
      if (isdigit(ch)) {
        td.text[p++] = ch;
      fnid_digits:
        for (;;) {
          ch = dbuf_getc(db);
          if (isdigit(ch)) {
            if (p < MAX_LEXEME - 1)
              td.text[p++] = ch;
          } else
            break;
        }
      }
      if (ch != '\0')
        dbuf_ungetc(db, 1);
      if (p > MAX_FNID_LEN) {
        td.kind = TK_ERROR;
        return td;
      }
      td.kind = TK_FUNID;
      return td;
    }
    if (isalpha(ch)) {
      td.text[p++] = ch;
      for (;;) {
        ch = dbuf_getc(db);
        if (isalpha(ch)) {
          if (p < MAX_LEXEME - 1)
            td.text[p++] = ch;
        } else
          break;
      }
      while (isdigit(ch)) {
        if (p < MAX_LEXEME - 1)
          td.text[p++] = ch;
        ch = dbuf_getc(db);
      }
      if (ch != '\0')
        dbuf_ungetc(db, 1);
      if (p > MAX_FNID_LEN) {
        td.kind = TK_ERROR;
        return td;
      }
      td.kind = TK_FUNID;
      return td;
    }
    if (ch != '\0')
      dbuf_ungetc(db, 1);
    td.kind = TK_ERROR;
    return td;

  default:
    break;
  }

  if (isdigit(ch)) {
    td.text[p++] = ch;
    for (;;) {
      ch = dbuf_getc(db);
      if (isdigit(ch)) {
        if (p < MAX_LEXEME - 1)
          td.text[p++] = ch;
      } else
        break;
    }
    if (ch == '.') {
      char nxt = dbuf_getc(db);
      if (isdigit(nxt)) {
        td.text[p++] = '.';
        td.text[p++] = nxt;
        char d2 = dbuf_getc(db);
        if (isdigit(d2)) {
          td.text[p++] = d2;
          char me = dbuf_getc(db);
          if (me == 'E') {
            td.text[p++] = 'E';
            char sd = dbuf_getc(db);
            if (sd == '+' || sd == '-') {
              td.text[p++] = sd;
              char e1 = dbuf_getc(db);
              if (isdigit(e1)) {
                td.text[p++] = e1;
                char e2 = dbuf_getc(db);
                if (isdigit(e2)) {
                  td.text[p++] = e2;
                  td.kind = TK_RNUM;
                  td.nv.rval = atof(td.text);
                  return td;
                }
                if (e2 != '\0')
                  dbuf_ungetc(db, 1);
                td.kind = TK_ERROR;
                return td;
              }
              if (e1 != '\0')
                dbuf_ungetc(db, 1);
              td.kind = TK_ERROR;
              return td;
            } else if (isdigit(sd)) {
              td.text[p++] = sd;
              char e2 = dbuf_getc(db);
              if (isdigit(e2)) {
                td.text[p++] = e2;
                td.kind = TK_RNUM;
                td.nv.rval = atof(td.text);
                return td;
              }
              if (e2 != '\0')
                dbuf_ungetc(db, 1);
              td.kind = TK_ERROR;
              return td;
            }
            if (sd != '\0')
              dbuf_ungetc(db, 1);
            td.kind = TK_ERROR;
            return td;
          }
          if (me != '\0')
            dbuf_ungetc(db, 1);
          td.kind = TK_RNUM;
          td.nv.rval = atof(td.text);
          return td;
        }
        if (d2 != '\0')
          dbuf_ungetc(db, 1);
        td.kind = TK_ERROR;
        return td;
      }
      if (nxt != '\0')
        dbuf_ungetc(db, 1);
      td.text[p++] = '.';
      td.kind = TK_ERROR;
      return td;
    }
    if (ch != '\0')
      dbuf_ungetc(db, 1);
    td.kind = TK_NUM;
    td.nv.ival = atoi(td.text);
    return td;
  }

  if (ch >= 'a' && ch <= 'z') {
    td.text[p++] = ch;
    if (is_bd(ch)) {
      char c2 = dbuf_getc(db);
      if (is_27(c2)) {
        td.text[p++] = c2;
        /* Pattern: [b-d][2-7][b-d]*[2-7]*.
         * After the mandatory prefix [b-d][2-7], lexer may consume:
         *   - more [b-d] chars (letter phase), then
         *   - [2-7] chars (digit-tail phase).
         * Once in digit-tail phase, seeing [b-d] ends current TK_ID;
         * that letter belongs to the next token.
         */
        int in_digit_tail = 0;
        for (;;) {
          ch = dbuf_getc(db);
          if (!in_digit_tail && is_bd(ch)) {
            if (p < MAX_LEXEME - 1)
              td.text[p++] = ch;
          } else if (is_27(ch)) {
            in_digit_tail = 1;
            if (p < MAX_LEXEME - 1)
              td.text[p++] = ch;
          } else {
            if (ch != '\0')
              dbuf_ungetc(db, 1);
            break;
          }
        }
        if (p < 2 || p > MAX_VARID_LEN) {
          td.kind = TK_ERROR;
          return td;
        }
        td.kind = TK_ID;
        return td;
      }
      if (c2 >= 'a' && c2 <= 'z') {
        td.text[p++] = c2;
        for (;;) {
          ch = dbuf_getc(db);
          if (ch >= 'a' && ch <= 'z') {
            if (p < MAX_LEXEME - 1)
              td.text[p++] = ch;
          } else {
            if (ch != '\0')
              dbuf_ungetc(db, 1);
            break;
          }
        }
        td.kind = rmap_find(rm, td.text);
        return td;
      }
      if (c2 != '\0')
        dbuf_ungetc(db, 1);
      td.kind = rmap_find(rm, td.text);
      return td;
    }
    for (;;) {
      ch = dbuf_getc(db);
      if (ch >= 'a' && ch <= 'z') {
        if (p < MAX_LEXEME - 1)
          td.text[p++] = ch;
      } else {
        if (ch != '\0')
          dbuf_ungetc(db, 1);
        break;
      }
    }
    td.kind = rmap_find(rm, td.text);
    return td;
  }

  td.text[0] = ch;
  td.kind = TK_ERROR;
  return td;
}

void strip_comments(char *src_path, char *dst_path) {
  FILE *fin = fopen(src_path, "r");
  FILE *fout = fopen(dst_path, "w");
  if (!fin || !fout) {
    if (fin)
      fclose(fin);
    if (fout)
      fclose(fout);
    fprintf(stderr, "Error: cannot open file for comment removal\n");
    return;
  }
  int inside = 0, c;
  while ((c = fgetc(fin)) != EOF) {
    if (c == '%') {
      inside = 1;
      continue;
    }
    if (c == '\n') {
      if (inside)
        inside = 0;
      fputc('\n', fout);
      continue;
    }
    if (!inside)
      fputc(c, fout);
  }
  fclose(fin);
  fclose(fout);
}
