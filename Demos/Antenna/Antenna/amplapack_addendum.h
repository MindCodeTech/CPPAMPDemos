/*----------------------------------------------------------------------------
 * Copyright © Microsoft Corporation. All rights reserved.
 *---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif
#define LAPACK_ZTRSM LAPACK_NAME(ztrsm,ZTRSM)
#define LAPACK_CTRSM LAPACK_NAME(ctrsm,CTRSM)
void LAPACK_ZTRSM(const char*, const char*, const char*, const char*, lapack_int*, lapack_int*, void*, void*, lapack_int*, void*, lapack_int*);
void LAPACK_CTRSM(const char*, const char*, const char*, const char*, lapack_int*, lapack_int*, void*, void*, lapack_int*, void*, lapack_int*);
#ifdef __cplusplus
}
#endif

void laswp(int n, ampblas::complex<double>* a, int lda, int k1, int k2, int* ipiv, int incx)
{
    LAPACK_ZLASWP(&n,a,&lda,&k1,&k2,ipiv,&incx);
}
void laswp(int n, ampblas::complex<float>* a, int lda, int k1, int k2, int* ipiv, int incx)
{
    LAPACK_CLASWP(&n,a,&lda,&k1,&k2,ipiv,&incx);
}
void trsm(char side, char uplo, char transa, char diag, int m, int n, ampblas::complex<double> alpha, ampblas::complex<double>* a, int lda, ampblas::complex<double>* b, int ldb)
{
    LAPACK_ZTRSM(&side,&uplo,&transa,&diag,&m,&n,&alpha,a,&lda,b,&ldb);
}
void trsm(char side, char uplo, char transa, char diag, int m, int n, ampblas::complex<float> alpha, ampblas::complex<float>* a, int lda, ampblas::complex<float>* b, int ldb)
{
    LAPACK_CTRSM(&side,&uplo,&transa,&diag,&m,&n,&alpha,a,&lda,b,&ldb);
}