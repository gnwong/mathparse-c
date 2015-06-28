/*  
 *  Copyright (C)  2015  George Wong
 *  GNU General Public License
 *
 */

#include "mpc.h"

int main (int argc, char **argv) {

  char line[80];

  // Demonstration of variable vectors
  double a[5] = {0.0, 1.0, 2.0, 3.0, 4.0};
  double b[5] = {0.0, 1.0, 2.0, 3.0, 4.0};
  double *out = NULL;
  strcpy(line, "a * b");
  printf("%s evalulates to %f\n", line, mathparse(line, 5, &out, "a", a, "b", b));
  for (int i=0; i<5; ++i) {
    printf("%d -> %f\n", i, out[i]);
  }
  free(out);

  // BC-like interpreter
  while (1) {
    gets(line);
    if (strcmp(line,"quit") == 0) break;
    printf("%f\n", mathparse(line));
  }

  return 0;
}