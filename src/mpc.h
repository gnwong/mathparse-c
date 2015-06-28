/*  
 *  Copyright (C)  2015  George Wong
 *  GNU General Public License
 *
 */

#ifndef MPC_H
#define MPC_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

// Definitions
enum TOKEN_TYPE { ERR, NUM, VAR, EXP, MUL, DIV, ADD, SUB, CTRL };

struct mpc_token {
  int type;
  int flag;
  double value;
  double *values;
  char *varname;
  struct mpc_token *prev;
  struct mpc_token *next;
};

// MathParse functions
double mathparse(const char *eqn, ...);
void mpc_free_token(struct mpc_token *tkn);
struct mpc_token *mpc_tokenize(const char *eqn, int *n_ops, int *n_others, int *n_vars);
void mpc_bop(struct mpc_token *one, struct mpc_token *two, struct mpc_token *bop, int n);

void mpc_print_token(struct mpc_token *tkn);
void mpc_print_token_chain(struct mpc_token *tkn);

#endif