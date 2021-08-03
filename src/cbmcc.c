#include "cbmcc.h"

#include <gsl/gsl_blas.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_matrix.h>
#include <time.h>

double trace(gsl_matrix *A) {
  double ret = 0.0;
  for (size_t i = 0; i < A->size1; ++i) ret += gsl_matrix_get(A, i, i);
  return ret;
}

double sgn(double x) { return x >= 0.0 ? 1.0 : -1.0; }

gsl_matrix *AAT(gsl_matrix *X) {
  gsl_matrix *C = gsl_matrix_calloc(X->size1, X->size1);
  // Does not affect the arguments
  gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, X, X, 0.0, C);
  return C;
}

gsl_matrix *init_W(gsl_matrix *X, size_t K) {
  gsl_matrix *XXT = AAT(X);

  gsl_eigen_symmv_workspace *workspace = gsl_eigen_symmv_alloc(XXT->size1);
  gsl_vector *eval = gsl_vector_alloc(XXT->size1);
  gsl_matrix *evec = gsl_matrix_alloc(XXT->size1, XXT->size1);

  // Sorting the eigen vectors

  // clock_t t1 = clock();
  gsl_eigen_symmv(XXT, eval, evec, workspace);
  gsl_eigen_symmv_sort(eval, evec, GSL_EIGEN_SORT_VAL_DESC);
  // printf("total seconds: %f\n", (double)(clock() - t1) / CLOCKS_PER_SEC);

  // Truncating W to have K columns (Needs improving maybe?)
  gsl_matrix *W = gsl_matrix_alloc(evec->size1, K);
  gsl_vector *aux = gsl_vector_alloc(evec->size1);
  for (size_t i = 0; i < K; i++) {
    gsl_matrix_get_col(aux, evec, i);
    gsl_matrix_set_col(W, i, aux);
  }
  gsl_vector_free(aux);

  gsl_eigen_symmv_free(workspace);
  gsl_vector_free(eval);
  gsl_matrix_free(evec);
  gsl_matrix_free(XXT);
  return W;
}

gsl_matrix *learn(MinutiaList *MCC_sets, size_t K, double bal_param, int T,
                  int convergence_param) {
  State state;
  initialize(&state);  // Maybe make only once

  gsl_matrix *X = ExtractRealFeatures(&state, MCC_sets);
  if (X == NULL) {
    return NULL;
  }

  int n_min = X->size2;  // NUmber of minutiae

  gsl_matrix *W = init_W(X, K);
  gsl_matrix *B = gsl_matrix_calloc(K, n_min);

  gsl_matrix *WTX = gsl_matrix_calloc(K, n_min);
  gsl_matrix *WTXXT = gsl_matrix_calloc(K, X->size1);
  gsl_matrix *WTXXTW = gsl_matrix_calloc(K, K);

  gsl_matrix *BTWT = gsl_matrix_calloc(n_min, X->size1);
  gsl_matrix *BTWTX = gsl_matrix_calloc(n_min, n_min);
  printf("AAAAAAA\n");

  double first_term = bal_param - 1.0 / n_min;
  for (int t = 0; t < T; t++) {
    // B = sgn(W^T . X)
    gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, W, X, 0.0, WTX);

    // Maybe there's a better way?
    for (size_t i = 0; i < B->size1; i++)
      for (size_t j = 0; j < B->size2; j++)
        gsl_matrix_set(B, i, j, sgn(gsl_matrix_get(WTX, i, j)));

    for (int i = 0; i < B->size1; i++) {
      printf("%f\t", gsl_matrix_get(B, i, 0));
    }

    // minj(W) = (lambda - 1/n) * tr(W^T . X . X^T . W) - 2 * lambda * tr(B^T .
    // W^T . X)
    gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, WTX, X, 0.0, WTXXT);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, WTXXT, W, 0.0, WTXXTW);

    gsl_blas_dgemm(CblasTrans, CblasTrans, 1.0, B, W, 0.0, BTWT);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, BTWT, X, 0.0, BTWTX);

    double minJW = first_term * trace(WTXXTW) - 2.0 * bal_param * trace(BTWTX);
    gsl_matrix_add_constant(W, minJW);  //?
  }

  gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, W, X, 0.0, B);
  for (size_t i = 0; i < B->size1; i++)
    for (size_t j = 0; j < B->size2; j++)
      gsl_matrix_set(B, i, j, sgn(gsl_matrix_get(B, i, j)));

  gsl_matrix_add_constant(B, 1.0);
  gsl_matrix_scale(B, 0.5);

  gsl_matrix_free(X);
  gsl_matrix_free(W);
  gsl_matrix_free(WTX);
  gsl_matrix_free(WTXXT);
  gsl_matrix_free(WTXXTW);
  gsl_matrix_free(BTWT);
  gsl_matrix_free(BTWTX);

  return B;
}

int main(int argc, char **argv) {
  MinutiaList list;
  ExtractData(&list, "../databases/DB2000/DB1-xyt/1_1.xyt");

  gsl_matrix *B = learn(&list, CBMCC_SIZE, 1.0, 1, 0.0);
  if (B == NULL) {
    printf("OOPS\n");
    return -1;
  }

  for (int i = 0; i < B->size1; i++) {
    printf("%f\t", gsl_matrix_get(B, i, 0));
  }
  printf("\n");

  gsl_matrix_free(B);

  return 0;
}