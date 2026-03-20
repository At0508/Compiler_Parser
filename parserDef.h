/*
 * Group 50
 * Samyak Savi          2022B3A70635P
 * Chavi Gupta          2022B3A70637P
 * Saumya Agarwal       2022B5A70943P
 * Atharva Agrawal      2022B3A70597P
 * Nishit Patel         2022B3A70568P
 * Agrim Goyal          2022B3A71269P
 */

#ifndef PARSER_DEF_H
#define PARSER_DEF_H

#include "lexerDef.h"

#define RULES_CAP 100
#define RHS_CAP 15
#define NUM_NT 54
#define SYM_EPS -2
#define CELL_SYNC -3
#define CELL_EMPTY -1

typedef enum {
  NT_program = 0,
  NT_mainFunction,
  NT_otherFunctions,
  NT_function,
  NT_input_par,
  NT_output_par,
  NT_parameter_list,
  NT_dataType,
  NT_primitiveDatatype,
  NT_constructedDatatype,
  NT_remaining_list,
  NT_stmts,
  NT_typeDefinitions,
  NT_actualOrRedefined,
  NT_typeDefinition,
  NT_fieldDefinitions,
  NT_fieldDefinition,
  NT_fieldType,
  NT_moreFields,
  NT_declarations,
  NT_declaration,
  NT_global_or_not,
  NT_otherStmts,
  NT_stmt,
  NT_assignmentStmt,
  NT_singleOrRecId,
  NT_option_single_constructed,
  NT_oneExpansion,
  NT_moreExpansions,
  NT_funCallStmt,
  NT_outputParameters,
  NT_inputParameters,
  NT_iterativeStmt,
  NT_conditionalStmt,
  NT_elsePart,
  NT_ioStmt,
  NT_arithmeticExpression,
  NT_expPrime,
  NT_term,
  NT_termPrime,
  NT_factor,
  NT_highPrecedenceOperators,
  NT_lowPrecedenceOperators,
  NT_booleanExpression,
  NT_logicalOp,
  NT_relationalOp,
  NT_returnStmt,
  NT_optionalReturn,
  NT_idList,
  NT_more_ids,
  NT_definetypestmt,
  NT_A,
  NT_var,
  NUM_NONTERMINALS
} NTerm;

static const char *nt_repr[]
    __attribute__((unused)) = {"program",
                               "mainFunction",
                               "otherFunctions",
                               "function",
                               "input_par",
                               "output_par",
                               "parameter_list",
                               "dataType",
                               "primitiveDatatype",
                               "constructedDatatype",
                               "remaining_list",
                               "stmts",
                               "typeDefinitions",
                               "actualOrRedefined",
                               "typeDefinition",
                               "fieldDefinitions",
                               "fieldDefinition",
                               "fieldType",
                               "moreFields",
                               "declarations",
                               "declaration",
                               "global_or_not",
                               "otherStmts",
                               "stmt",
                               "assignmentStmt",
                               "singleOrRecId",
                               "option_single_constructed",
                               "oneExpansion",
                               "moreExpansions",
                               "funCallStmt",
                               "outputParameters",
                               "inputParameters",
                               "iterativeStmt",
                               "conditionalStmt",
                               "elsePart",
                               "ioStmt",
                               "arithmeticExpression",
                               "expPrime",
                               "term",
                               "termPrime",
                               "factor",
                               "highPrecedenceOperators",
                               "lowPrecedenceOperators",
                               "booleanExpression",
                               "logicalOp",
                               "relationalOp",
                               "returnStmt",
                               "optionalReturn",
                               "idList",
                               "more_ids",
                               "definetypestmt",
                               "A",
                               "var"};

typedef struct {
  int isTerm;
  int idx;
} Symbol;

typedef struct {
  NTerm head;
  Symbol body[RHS_CAP];
  int bodyLen;
} Production;

typedef struct {
  Production prods[RULES_CAP];
  int nprods;
} RuleSet;

typedef struct {
  int fst[NUM_NONTERMINALS][NUM_TOKENS];
  int flw[NUM_NONTERMINALS][NUM_TOKENS];
  int nullable[NUM_NONTERMINALS];
} FFSets;

typedef struct {
  int entry[NUM_NONTERMINALS][NUM_TOKENS];
} PredTable;

typedef struct PNode {
  int leaf;
  TokData tok;
  NTerm nt;
  int prodIdx;
  struct PNode *up;
  struct PNode *child;
  struct PNode *sib;
} PNode;

typedef struct SymFrame {
  Symbol sym;
  PNode *node;
  struct SymFrame *prev;
} SymFrame;

typedef struct {
  SymFrame *head;
} SymStack;

#endif
