/* 

  starfit.h - Stan Thomas 8-14-2011

  starfit header.

*/

#define NRANSI

#define EPSILON (0.02e0)
#define NSTARS 5000
#define NSITES 4
#define NMIRS  29
#define D2R  (M_PI/180.0e0)
#define R2D  (180.0e0/M_PI)

#define dot(A,B)  (((A).e)*((B).e)+((A).n)*((B).n)+((A).u)*((B).u))
#define mag(A)    sqrt(dot(A,A))

typedef struct VECTOR
{
  double e, n, u;
}VECTOR;

double gasdev( void);
void starpos(int istar, double *xstar, double *ystar, float x[]);
void amoeba(float **p, float y[], int ndim, float ftol, float (*funk)(float []), int *nfunk);
float chi2_func(float x[]);
double fit_func(int N, double param[], float (*funk)(float []));


