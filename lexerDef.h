/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#ifndef LEXER_DEF_H
#define LEXER_DEF_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBUF_SIZE 512
#define MAX_LEXEME 128
#define MAX_VARID_LEN 20
#define MAX_FNID_LEN 30
#define RMAP_BUCKETS 64

typedef enum {
  TK_ASSIGNOP,
  TK_COMMENT,
  TK_FIELDID,
  TK_ID,
  TK_NUM,
  TK_RNUM,
  TK_FUNID,
  TK_RUID,
  TK_WITH,
  TK_PARAMETERS,
  TK_END,
  TK_WHILE,
  TK_UNION,
  TK_ENDUNION,
  TK_DEFINETYPE,
  TK_AS,
  TK_TYPE,
  TK_MAIN,
  TK_GLOBAL,
  TK_PARAMETER,
  TK_LIST,
  TK_SQL,
  TK_SQR,
  TK_INPUT,
  TK_OUTPUT,
  TK_INT,
  TK_REAL,
  TK_COMMA,
  TK_SEM,
  TK_COLON,
  TK_DOT,
  TK_ENDWHILE,
  TK_OP,
  TK_CL,
  TK_IF,
  TK_THEN,
  TK_ENDIF,
  TK_READ,
  TK_WRITE,
  TK_RETURN,
  TK_PLUS,
  TK_MINUS,
  TK_MUL,
  TK_DIV,
  TK_CALL,
  TK_RECORD,
  TK_ENDRECORD,
  TK_ELSE,
  TK_AND,
  TK_OR,
  TK_NOT,
  TK_LT,
  TK_LE,
  TK_EQ,
  TK_GT,
  TK_GE,
  TK_NE,
  TK_EOF,
  TK_ERROR,
  NUM_TOKENS
} TokKind;

static const char *tok_repr[] __attribute__((unused)) = {
    "TK_ASSIGNOP", "TK_COMMENT",   "TK_FIELDID", "TK_ID",       "TK_NUM",
    "TK_RNUM",     "TK_FUNID",     "TK_RUID",    "TK_WITH",     "TK_PARAMETERS",
    "TK_END",      "TK_WHILE",     "TK_UNION",   "TK_ENDUNION", "TK_DEFINETYPE",
    "TK_AS",       "TK_TYPE",      "TK_MAIN",    "TK_GLOBAL",   "TK_PARAMETER",
    "TK_LIST",     "TK_SQL",       "TK_SQR",     "TK_INPUT",    "TK_OUTPUT",
    "TK_INT",      "TK_REAL",      "TK_COMMA",   "TK_SEM",      "TK_COLON",
    "TK_DOT",      "TK_ENDWHILE",  "TK_OP",      "TK_CL",       "TK_IF",
    "TK_THEN",     "TK_ENDIF",     "TK_READ",    "TK_WRITE",    "TK_RETURN",
    "TK_PLUS",     "TK_MINUS",     "TK_MUL",     "TK_DIV",      "TK_CALL",
    "TK_RECORD",   "TK_ENDRECORD", "TK_ELSE",    "TK_AND",      "TK_OR",
    "TK_NOT",      "TK_LT",        "TK_LE",      "TK_EQ",       "TK_GT",
    "TK_GE",       "TK_NE",        "TK_EOF",     "TK_ERROR"};

typedef union {
  int ival;
  double rval;
} NumVal;

typedef struct {
  TokKind kind;
  char text[MAX_LEXEME];
  int ln;
  NumVal nv;
} TokData;

typedef struct {
  char bufA[DBUF_SIZE];
  char bufB[DBUF_SIZE];
  int activeSide;
  int pos;
  int lenA;
  int lenB;
  int line;
  FILE *fp;
  int atEnd;
} DualBuf;

typedef struct ResEntry {
  char name[32];
  TokKind kind;
  struct ResEntry *next;
} ResEntry;

typedef struct {
  ResEntry *slots[RMAP_BUCKETS];
} ReservedMap;

#endif
