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
#include "parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GRAMMAR_PATH "grammar.txt"

static void show_status() {
  printf("\n===== IMPLEMENTATION STATUS =====\n");
  printf("  (a) FIRST and FOLLOW sets automated\n");
  printf("  (b) Lexical analyzer module developed\n");
  printf("  (c) Both lexical and syntax analysis modules implemented\n");
  printf("  (d) Parse tree constructed upon successful parsing\n");
  printf("  (e) Panic mode error recovery with sync sets\n");
  printf("=================================\n\n");
}

static int varid_exceeds_limit(const char *lex) {
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

static int single_unknown_char(const char *lex) {
  if (strlen(lex) != 1)
    return 0;
  unsigned char c = (unsigned char)lex[0];
  return !(isalnum(c) || c == '_' || c == '#');
}

static void report_lex_error(const TokData *td) {
  if (varid_exceeds_limit(td->text)) {
    printf("Line No %d: Error :Variable Identifier is longer than the "
           "prescribed length of 20 characters.\n",
           td->ln);
    return;
  }
  if (single_unknown_char(td->text)) {
    if (strcmp(td->text, "=") == 0) {
      printf("Line No %d: Error : Unknown Symbol <%s>\n", td->ln, td->text);
      return;
    }
    printf("Line No %d : Error: Unknown Symbol <%s>\n", td->ln, td->text);
    return;
  }
  printf("Line no: %d : Error: Unknown pattern <%s>\n", td->ln, td->text);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <testcase_file> <parseTreeOutFile>\n", argv[0]);
    return 1;
  }

  char *src = argv[1];
  char *dst = argv[2];

  show_status();

  RuleSet rs = ruleset_load(GRAMMAR_PATH);
  if (rs.nprods == 0) {
    fprintf(stderr, "Fatal: grammar not loaded. Ensure '%s' exists.\n",
            GRAMMAR_PATH);
    return 1;
  }
  FFSets ff = ff_compute(&rs);
  PredTable pt;
  ptable_build(&ff, &pt, &rs);

  ReservedMap rm;
  rmap_init(&rm);

  int choice = -1;
  while (choice != 0) {
    printf("\nSelect an option:\n");
    printf("  0 : Exit\n");
    printf("  1 : Remove comments and print clean code\n");
    printf("  2 : Print token list (lexer only)\n");
    printf("  3 : Parse and print parse tree\n");
    printf("  4 : Measure total CPU time for lexer + parser\n");
    printf("Enter choice: ");
    if (scanf("%d", &choice) != 1)
      break;

    switch (choice) {
    case 0:
      printf("Exiting.\n");
      break;

    case 1: {
      strip_comments(src, "commentFreeCode.txt");
      FILE *cf = fopen("commentFreeCode.txt", "r");
      if (cf) {
        printf("\n--- Comment-free code ---\n");
        int c;
        while ((c = fgetc(cf)) != EOF)
          putchar(c);
        printf("\n--- End ---\n");
        fclose(cf);
      }
      break;
    }

    case 2: {
      FILE *fp = fopen(src, "r");
      if (!fp) {
        fprintf(stderr, "Cannot open %s\n", src);
        break;
      }
      DualBuf db;
      dbuf_open(&db, fp);
      printf("\n");
      for (;;) {
        TokData td = scan_next(&db, &rm);
        if (td.kind == TK_EOF)
          break;
        if (td.kind == TK_ERROR)
          report_lex_error(&td);
        else
          printf("Line no. %d\t Lexeme %s\t\tToken %s\n", td.ln, td.text,
                 tok_repr[td.kind]);
      }
      fclose(fp);
      break;
    }

    case 3: {
      int ec = 0;
      PNode *tree = parse_source(src, &pt, &rs, &rm, &ec);
      if (tree) {
        ptree_dump(tree, dst);
        if (ec == 0)
          printf("Parse tree written to '%s'\n", dst);
        else
          printf("Parsing completed with %d error(s). Partial parse tree in "
                 "'%s'\n",
                 ec, dst);
        ptree_free(tree);
      }
      break;
    }

    case 4: {
      clock_t t0 = clock();
      int ec = 0;
      PNode *tree = parse_source(src, &pt, &rs, &rm, &ec);
      clock_t t1 = clock();
      double ticks = (double)(t1 - t0);
      printf("\nTotal CPU time (clock ticks) : %.0f\n", ticks);
      printf("Total CPU time (seconds)     : %f\n", ticks / CLOCKS_PER_SEC);
      if (tree)
        ptree_free(tree);
      break;
    }

    default:
      printf("Invalid option.\n");
      break;
    }
  }

  rmap_destroy(&rm);
  return 0;
}
