#ifndef _CBMCC_
#define _CBMCC_

#include <gsl/gsl_matrix.h>

#define CBMCC_SIZE 256
#define CBMCC_VALUE_SIZE ((CBMCC_SIZE + 31) / 32)

typedef struct {
  int values[MCC_VALUE_SIZE];
  Minutia minutia;
  int popcount;
  int template_index;  // i
  int minutiae_index;  // j
  char *file;
} CBMCC;

gsl_matrix *AAT(gsl_matrix *X);
gsl_matrix *learn(MinutiaList *MCC_sets, size_t K, double bal_param, int T,
                  int convergence_param);

#endif /* _CBMCC_ */