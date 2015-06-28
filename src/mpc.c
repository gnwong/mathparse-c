/*
 *  `mpc.c'
 *    Math Equation Parser for C
 *
 *  Copyright (C)  2015  George Wong
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see http://www.gnu.org/licenses/.
 *
 */

#include "mpc.h"

double mathparse (const char *ineqn, ...) {

  int i,j;
  int eqnlen;
  int vectorsize = 0;
  double **vectorout = NULL;

  // Strip of whitespace
  j = 0;
  eqnlen = strlen(ineqn);
  char *eqn = (char *)malloc(eqnlen * sizeof(char));
  for (i=0; i<eqnlen; ++i) {
    if (ineqn[i]!=' ' && ineqn[i]!='\t') {
      eqn[j] = ineqn[i];
      j++;
    }
  }
  eqn[j] = '\0';

  if (strlen(eqn) < 1) return NAN; // TODO Should this display error message?

  // Tokenize
  int n_ops = 0;
  int n_vars = 0;
  int n_others = 0;
  struct mpc_token *token_head = mpc_tokenize(eqn, &n_ops, &n_others, &n_vars);

  // Attach input to variables (if applicable)
  if (n_vars != 0) {
    va_list list;
    va_start(list, ineqn);
    vectorsize = va_arg(list, int);
    vectorout = va_arg(list, double**);
    for (i=0; i<n_vars; ++i) {
      char   *tmp_name = va_arg(list, char*);
      double *tmp_loc  = va_arg(list, double*);
      struct mpc_token *t = token_head;
      while (t != NULL) {
        if (t->type == VAR) {
          if (strcmp(t->varname, tmp_name) == 0) {
            t->values = (double *)malloc(sizeof(double) * vectorsize);
            memcpy(t->values, tmp_loc, sizeof(double) * vectorsize);
          }
        }
        t = t->next;
      }
    }
    va_end(list);
  }

  // Sort into reverse polish order
  i = 0;
  struct mpc_token *op_stack[n_ops];
  struct mpc_token *c = token_head;
  struct mpc_token *rpn = NULL;
  while (c != NULL) {
    if (c->type == NUM || c->type == VAR) {
      if (rpn != NULL) rpn->next = c;
      c->prev = rpn;
      rpn = c;
    } else {
      if (i == 0) {
        op_stack[i] = c;
        i++;
      } else {
        if (c->type == CTRL) {
          if (c->flag == 1 || c->flag == 3) {
            op_stack[i] = c;
            i++;
          } else {
            while (op_stack[i-1]->type != CTRL || op_stack[i-1]->flag != (c->flag-1)) {
              rpn->next = op_stack[i-1];
              op_stack[i-1]->prev = rpn;
              rpn = op_stack[i-1];
              i--;
            }
            i--;
          }
        } else {
          while (i>0 && op_stack[i-1]->type != CTRL && op_stack[i-1]->flag >= c->flag) {
            rpn->next = op_stack[i-1];
            op_stack[i-1]->prev = rpn;
            rpn = op_stack[i-1];
            i--;
          }
          op_stack[i] = c;
          i++;
        }
      }
    }
    c = c->next;
  }

  while (i > 0) {
    rpn->next = op_stack[i-1];
    op_stack[i-1]->prev = rpn;
    rpn = op_stack[i-1];
    i--;
  }
  rpn->next = NULL;
  while (rpn->prev != NULL) {
    rpn = rpn->prev;
  }

  // Evaluate
  i = 0;
  struct mpc_token *val_stack[n_others];
  c = rpn;
  while (c != NULL) {

    if (c->type == NUM || c->type == VAR) {
      val_stack[i] = c;
      i++;
    } else {
      if (i<2) return NAN; // TODO Should this display an error message?
      mpc_bop(val_stack[i-2], val_stack[i-1], c, vectorsize);
      i--;
    }

    c = c->next;
  }

  // Set vectorout if necessary
  if (n_vars != 0) {
    *vectorout = (double *)malloc(sizeof(double) * vectorsize);
    memcpy(*vectorout, val_stack[0]->values, sizeof(double) * vectorsize);
  }

  // Perform housekeeping
  double retval = val_stack[0]->value;
  while (rpn != NULL) {
    c = rpn;
    rpn = rpn->next;
    mpc_free_token(c);
  }

  return retval;
}

struct mpc_token *mpc_tokenize (const char *eqn, int *n_ops, int *n_others, int *n_vars) {

  int i;
  int c_token_type;
  int c_token_flag;
  struct mpc_token *current = NULL;
  struct mpc_token *last = NULL;

  *n_ops = 0; *n_others = 0; *n_vars = 0;

  for (i=0; i<strlen(eqn); ++i) {

    // Parse here
    switch (eqn[i]) {

    case '(':
      c_token_type = CTRL;
      c_token_flag = 1;
      break;

    case ')':
      c_token_type = CTRL;
      c_token_flag = 2;
      break;

    case '[':
      c_token_type = CTRL;
      c_token_flag = 3;
      break;

    case ']':
      c_token_type = CTRL;
      c_token_flag = 4;
      break;

      // TODO support { } ?

    case '^':
      c_token_type = EXP;
      c_token_flag = 3;
      break;

    case '*':
      c_token_type = MUL;
      c_token_flag = 2;
      break;

    case '/':
      c_token_type = DIV;
      c_token_flag = 2;
      break;

    case '+':
      c_token_type = ADD;
      c_token_flag = 1;
      break;

    case '-':
      c_token_type = SUB;
      c_token_flag = 1;
      break;

    case '.':
      c_token_type = NUM;
      c_token_flag = 10;
      break;

    default:
      // Parse numbers
      if (eqn[i] > 47 && eqn[i] < 58) {
        c_token_type = NUM;
        c_token_flag = eqn[i] - 48;
      }

      // Parse letters
      else if ((eqn[i] < 123 && eqn[i] > 96) || (eqn[i] < 91 && eqn[i] > 64)) {
        c_token_type = VAR;
        c_token_flag = eqn[i];
      } 

      // Otherwise, print this, because we don't know what to do
      else {
        printf("%c -> %d\n", eqn[i], eqn[i]);  
      }

    }

    // Update token here
    if (current == NULL && c_token_type == NUM) ;
    else if (c_token_type == NUM || c_token_type == VAR) {
      if (c_token_type == NUM && current->type != VAR) ;
      else if (c_token_type == NUM && current->type == VAR) {
        c_token_type = VAR;
        char *tmp = (char *)malloc(sizeof(char) * (strlen(current->varname)+1));
        memcpy(tmp, current->varname, (strlen(current->varname)+2)*sizeof(char));
        free(current->varname);
        tmp[strlen(tmp)+1] = '\0';
        tmp[strlen(tmp)] = c_token_flag + 48;
        current->varname = tmp;
      } else {
        if (current == NULL || current->type != VAR) {
          last = current;
          current = (struct mpc_token *)malloc(sizeof(struct mpc_token));
          *n_others += 1;
          current->value = 0.0;
          current->flag = 0;
          current->type = VAR;
          current->prev = last;
          current->values = NULL;
          if (last != NULL) last->next = current;
          current->varname = (char *)malloc(sizeof(char)*2);
          current->varname[0] = c_token_flag;
          current->varname[1] = '\0';
        } else {
          char *tmp = (char *)malloc(sizeof(char) * (strlen(current->varname)+1));
          memcpy(tmp, current->varname, (strlen(current->varname)+2)*sizeof(char));
          free(current->varname);
          tmp[strlen(tmp)+1] = '\0';
          tmp[strlen(tmp)] = c_token_flag;
          current->varname = tmp;
        }
      }
    }

    if (c_token_type == NUM) {
      if (current == NULL || current->type != NUM) {
        last = current;
        current = (struct mpc_token *)malloc(sizeof(struct mpc_token));
        *n_others += 1;
        current->value = 0.0;
        current->flag = -1;
        current->type = NUM;
        current->prev = last;
        current->values = NULL;
        current->varname = NULL;
        if (last != NULL) last->next = current;
      }

      if (c_token_flag == 10) {
        current->flag = 0;
      } else if (current->flag > -1) {
        current->flag += 1;
        current->value += (double)c_token_flag / pow(10,current->flag);
      } else {
        current->value *= 10;
        current->value += (double)c_token_flag;
      }
    } else if (c_token_flag < 5 && c_token_flag > 0) {
      last = current;
      current = (struct mpc_token *)malloc(sizeof(struct mpc_token));
      if (c_token_type != CTRL) *n_ops += 1;
      if (last != NULL) last->next = current;
      current->prev = last;

      current->type = c_token_type;
      current->flag = c_token_flag;
      current->value = 0.0;
      current->values = NULL;
      current->varname = NULL;
    }
    
  }

  // Find variables
  char *varnames[strlen(eqn)];
  while (current->prev != NULL) current = current->prev;
  while (1) {
    if (current->type == VAR) {
      int found = 0;
      for (i=0; i<*n_vars; ++i) {
        if (strcmp(varnames[i], current->varname) == 0) {
          found = 1; break;
        }
      }
      if (found != 1) {
        varnames[*n_vars] = current->varname;
        *n_vars += 1;
      }
    }
    if (current->next == NULL) break;
    current = current->next;
  }

  // Reset list to head
  while (current->prev != NULL) current = current->prev;

  return current;
}

void mpc_free_token (struct mpc_token *tkn) {
  if (tkn->values != NULL) free(tkn->values);
  if (tkn->varname != NULL) free(tkn->varname);
  free(tkn);
}

void mpc_bop (struct mpc_token *one, struct mpc_token *two, struct mpc_token *bop, int n) {
 
  int i; 
  int var1 = (one->type==VAR) ? 1:0;
  int var2 = (two->type==VAR) ? 1:0;

  switch (bop->type) {

  case EXP:
    if (var1 != 1 && var2 != 1) {
      one->value = pow(one->value, two->value);
    } else {
      for (i=0; i<n; ++i) {
        if (var1 & var2) one->values[i] = pow(one->values[i], two->values[i]);
        else if (var2) two->values[i] = pow(one->value, two->values[i]);
        else one->values[i] = pow(one->values[i],two->value);
      }
    }
    break;
    
  case MUL:
    if (var1 != 1 && var2 != 1) {
      one->value *= two->value;
    } else {
      for (i=0; i<n; ++i) {
        if (var1 & var2) one->values[i] *= two->values[i];
        else if (var2) two->values[i] *= one->value;
        else one->values[i] *= two->value;
      }
    }
    break;

  case DIV:
    if (var1 != 1 && var2 != 1) {
      one->value /= two->value;
    } else {
      for (i=0; i<n; ++i) {
        if (var1 & var2) one->values[i] /= two->values[i];
        else if (var2) two->values[i] = one->value / two->values[i];
        else one->values[i] /= two->value;
      }
    }
    break;

  case ADD:
    if (var1 != 1 && var2 != 1) {
      one->value += two->value;
    } else {
      for (i=0; i<n; ++i) {
        if (var1 & var2) one->values[i] += two->values[i];
        else if (var2) two->values[i] += one->value;
        else one->values[i] += two->value;
      }
    }
    break;

  case SUB:
    if (var1 != 1 && var2 != 1) {
      one->value -= two->value;
    } else {
      for (i=0; i<n; ++i) {
        if (var1 & var2) one->values[i] -= two->values[i];
        else if (var2) two->values[i] *= one->value - two->values[i];
        else one->values[i] -= two->value;
      }
    }
    break;

  default: ;
  }

  if (one->type != VAR && two->type == VAR) {
    struct mpc_token *tmp = one;
    one = two;
    two = tmp;
  }

}


// Debug functions
void mpc_print_token (struct mpc_token *tkn) {
  
  if (tkn == NULL) {
    fprintf(stderr, "No token!\n");
    return;
  }

  switch (tkn->type) {
  case NUM:
    fprintf(stderr, "%f ", tkn->value);
    break;
  case EXP:
    fprintf(stderr, "^ ");
    break;
  case MUL:
    fprintf(stderr, "* ");
    break;
  case DIV:
    fprintf(stderr, "/ ");
    break;
  case ADD:
    fprintf(stderr, "+ ");
    break;
  case SUB:
    fprintf(stderr, "- ");
    break;
  case VAR:
    fprintf(stderr, "'%s' ", tkn->varname);
    break;
  case CTRL:
    switch (tkn->flag) {
    case 1:
      fprintf(stderr, "( ");
      break;
    case 2:
      fprintf(stderr, ") ");
      break;
    case 3:
      fprintf(stderr, "[ ");
      break;
    case 4:
      fprintf(stderr, "] ");
      break;
    default:
      fprintf(stderr, "Unknown control token\n");
    }
    break;
  default:
    fprintf(stderr, "Token of type: %d\n", tkn->type);
    fprintf(stderr, "         flag: %d\n", tkn->flag);
    fprintf(stderr, "        value: %f\n", tkn->value);
  }
}

void mpc_print_token_chain (struct mpc_token *tkn) {
  if (tkn == NULL) return;
  while (tkn->prev != NULL) tkn = tkn->prev;

  while (tkn != NULL) {
    mpc_print_token(tkn);
    tkn = tkn->next;
  }
  fprintf(stderr, "\n");
}
