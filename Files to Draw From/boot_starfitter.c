/* 

  Starfit.c - Stan Thomas 08-16-2011

  Routine for photographic surveying.

----------------------------------------------------------------------------
  
----------------------------------------------------------------------------

*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "new_starfitter.h"
#include "nrutil.h"

#define NBOOT 5

void print_chi2(float x[]);
int reject_star(float x[]);

VECTOR s[NSTARS], s0[NSTARS];
double xmeas[NSTARS], ymeas[NSTARS];
double sigmax[NSTARS], sigmay[NSTARS];
char sname[NSTARS][12];

const int MAXMIR[NSITES] = { 24, 11, 11}; 
const int MINMIR[NSITES] = { 1, 0, 0};
const double CUTX[NSITES] = {11.5e0, 19.0e0, 19.0e0};  // Cut on x position
const double CUTY[NSITES] = {10.7e0, 16.0e0, 16.0e0};  // Cut on y position
const double CUTD = 0.5e0;  // Absolute cut on initial error          

double azi[NSITES][NMIRS];  // Cluster azimuth angle
double elev[NSITES][NMIRS]; // Cluster elevation angle
double rot[NSITES][NMIRS];  // Cluster rotation angle
double mcd[NSITES][NMIRS];  // Mirror-cluster distances (meters)
double Rmir[NSITES][NMIRS]; // Mirror radius of curvature (meters)

int Nstars = 0;
int NPARMS = 0;

double xglobal[9];   // Global storage of x parameter vector

int site, mir;

int main(int argc, char *argv[])
{
  FILE *fp, *op;
  static double parms[10];  // Maximum number of parameters
  float x[10];
  int i, iboot, istar;
  double stheta, sphi;
  double xstar, ystar;
  double theta, phi, psi, x0, y0, D0;
  double chi2min;
  char buffer[1025];
  int year, month, day, hour, minutes, exposure;
  double seconds, delta;
  int iter;
  int Nstars0 = 0;
  char geofile[256];

  srand48(time(NULL));

  if(argc<4)
    {
      fprintf(stderr,"Usage: %s NPARMS <mirror_geometry.dat> <*.stars.txt>\7\7\n", argv[0]);
      exit(-1);
    }

  NPARMS = atoi(argv[1]);

  if(NPARMS<2)
    {
      fprintf(stderr,"ERROR: Insufficient number of parameters for fit NPARMS = %s\7\7\n", argv[1]);
      fprintf(stderr,"Usage: %s NPARMS <mirror_geometry.dat> <*.stars.txt>\7\7\n", argv[0]);
      exit(-1);
    }

  if(NPARMS>6)
    {
      fprintf(stderr,"ERROR: Too many parameters for fit NPARMS = %s\7\7\n", argv[1]);
      fprintf(stderr,"Usage: %s NPARMS <mirror_geometry.dat> <*.stars.txt>\7\7\n", argv[0]);
      exit(-1);
    }

  strncpy(geofile,argv[2],255);

  if((op=fopen(geofile,"r"))==NULL)
    {
      fprintf(stderr,"Unable to open mirror geometry file: \"%s\"\7\7\n", argv[2]);
      exit(-1);
    }

  while(fgets(buffer,1023,op)!=NULL)
    {
      if(sscanf(buffer,"%d %d", &site, &mir) != 2)
	{
	  fprintf(stderr,"Error reading mirror geometry from \"%s\"\7\7\n", geofile);
	  exit(-1);
	}

      if(sscanf(buffer,"%*d %*d %lf %lf %lf %lf %lf", &azi[site][mir], &elev[site][mir], 
		&rot[site][mir], &mcd[site][mir], &Rmir[site][mir])!=5)
	{
	  fprintf(stderr,"Error reading mirror geometry from \"%s\"\7\7\n", geofile);
	  exit(-1);
	}
    }

  fclose(op);

  // Correct azimuths for fitting

  for(site=0;site<NSITES;site++)
    {
      for(mir=0;mir<NMIRS;mir++)
        {
          azi[site][mir] = 90.0e0 - azi[site][mir];
          if(azi[site][mir] < 0.0e0)azi[site][mir] += 360.0e0;
        }
    }

  // Read in the matched star data

  if((fp=fopen(argv[3],"r"))==NULL)
    {
      fprintf(stderr,"Unable to open star file \"%s\"\7\7\n", argv[3]);
      exit(-1);
    }


  Nstars = 0;
  while(fgets(buffer,1023,fp)!=NULL)
    {
      if(sscanf(buffer," %d %d %d/%d/%d %d:%d:%lf %d %s %lf %lf %lf %lf %lf %lf %lf",
		&site, &mir, 
		&year, &month, &day, &hour, &minutes, &seconds, &exposure,
		sname[Nstars],
		&stheta, &sphi, 
		&xstar, &ystar,
		&xmeas[Nstars], &ymeas[Nstars],
		&delta) != 17)continue;
      
      site = site-1;
      if((site<0)||(site>NSITES))
	{
	  fprintf(stderr,"Invalid site number\7\7\n");
	  exit(-1);
	}

      if((mir<MINMIR[site])||(mir>MAXMIR[site]))
	{
	  fprintf(stderr,"Invalid mirror number\7\7\n");
	  exit(-1);
	}

      stheta *= D2R;
      sphi *= D2R;

      s0[Nstars].e = cos(stheta)*cos(sphi);
      s0[Nstars].n = cos(stheta)*sin(sphi);
      s0[Nstars].u = sin(stheta);
      
      sigmax[Nstars] = 0.081e0;
      sigmay[Nstars] = 0.081e0;

      // Apply quality cuts to throw out stars that are likely to have been poorly measured.
      // This includes stars that are too close to the edge of the cameras. If part of the 
      // spot is cut off at the edge of the mirror it will cause the centroid to be mis-measured.
      // These cuts were determined by looking at the residuals in the data.
      // The cut on delta was determined by looking at the delta distribution for all stars and
      // all mirrors. Hopefully, this will help to throw out stars where the centroid was mis-measured
      // because they are partially darkened by Tom's wooden support bars.

      if(fabs(xstar)>CUTX[site])continue;
      if(fabs(ystar)>CUTY[site])continue;
      if(fabs(xmeas[Nstars])>CUTX[site])continue;
      if(fabs(ymeas[Nstars])>CUTY[site])continue;
      //      if(fabs(delta)>CUTD)continue;

      Nstars++;
    }

  fclose(fp);

  if(Nstars<NPARMS+1)
    {
      fprintf(stderr,"Insufficient stars N = %d found.\7\7\n", Nstars);
      exit(-1);
    }

  // Bootstrap iterations

  for(iboot=0;iboot<NBOOT;iboot++)
    {

      // Start by selecting a bootstrap set of stars

      for(i=0;i<Nstars;i++)
	{
	  istar = rand()%Nstars; // Choose random star - with replacement
	  s[i].e = s0[istar].e;
	  s[i].n = s0[istar].n;
	  s[i].u = s0[istar].u;
	}

      // Choose new starting parameters

      parms[0] = theta = elev[site][mir]*D2R;
      parms[1] = phi   = azi[site][mir]*D2R;
      
      parms[2] = D0 = (Rmir[site][mir] - mcd[site][mir])*100.0e0/2.54e0;
      
      if(NPARMS>3)
	parms[3] = psi   = drand48()-0.5e0;
      else
	parms[3] = 0.0e0;
      
      
      if(NPARMS>4)
	parms[4] = x0 = drand48()-0.5e0;
      else
	parms[4] = x0 = 0.0e0;
      
      if(NPARMS>5)
	parms[5] = y0 = drand48()-0.5e0;
      else
	parms[5] = y0 = 0.0e0; 
      
      for(i=0;i<6;i++)xglobal[i+1] = (float)parms[i]; // Save all parameters globally
      for(i=0;i<NPARMS;i++)x[i+1] = (float)parms[i]; // Save fit parameters

      for(iter=0;iter<100;iter++)
	{
	  Nstars0 = Nstars;
	  if(Nstars<NPARMS+1)
	    {
	      fprintf(stderr,"Insufficient stars N = %d found.\7\7\n", Nstars);
	      exit(-1);
	    }
	  
	  chi2min = fit_func(NPARMS, parms, chi2_func);
	  for(i=0;i<NPARMS;i++)x[i+1] = (float)parms[i];
	  
	  if(Nstars0 == reject_star(x))break;
	}

      for(iter=0;iter<4;iter++)
	{
	  if(Nstars<NPARMS+1)
	    {
	      fprintf(stderr,"Insufficient stars N = %d found.\7\7\n", Nstars);
	      exit(-1);
	    }
	  
	  chi2min = fit_func(NPARMS, parms, chi2_func);
	  for(i=0;i<NPARMS;i++)x[i+1] = (float)parms[i];
	}

      theta = parms[0]*R2D;
      phi   = 90.0e0 - parms[1]*R2D;
      if(phi<0.0e0)phi += 360.0e0;
      D0    = parms[2];
      psi   = parms[3]*R2D;
      x0    = parms[4];
      y0    = parms[5];
      
      for(i=0;i<NPARMS;i++)x[i+1] = (float)parms[i];
      
      printf("%d %d ", site, mir);
      printf("%.3lf ", phi);
      printf("%.3lf ", theta);
      printf("%.3lf ", psi);
      if(NPARMS>4)printf("%.3lf ", x0);
      if(NPARMS>5)printf("%.3lf ", y0);
      printf("%.3lf ", mcd[site][mir]);
      printf("%.3lf ", mcd[site][mir]+D0*2.54e0/100.0e0);
      printf("%d ", Nstars);
      printf("%.8lf\n", chi2min);
      fflush(stdout);

    } // Next bootstrap iteration

  return 0;
}

int reject_star(float x[])
{
  double derr, xstar, ystar;
  int i, itmp, ierr = (-1);
  double maxderr = 0.0e0;
  double MAXERR = 0.235e0;

  if(Nstars<NPARMS+1)return Nstars;

  for(i=0;i<Nstars;i++)
    {
      starpos(i, &xstar, &ystar, x);

      derr  = pow(xmeas[i] - xstar,2.0e0);
      derr += pow(ymeas[i] - ystar,2.0e0);
      derr = sqrt(derr);
      if(derr>maxderr)
	{
	  maxderr = derr;
	  ierr = i;
	}
    }

  // Reject the worst star if the error is greater than 0.235"

  if(NPARMS<3)MAXERR = 5.0e0;
  if(NPARMS==3)MAXERR = 2.5e0;
  if(NPARMS>=4)MAXERR = 0.235e0;

  if(maxderr>MAXERR)
    {
      fprintf(stderr,"Rejected star %d. %s\n", ierr, sname[ierr]);
      xmeas[ierr] = xmeas[Nstars-1];
      ymeas[ierr] = ymeas[Nstars-1];
      s[ierr].e = s[Nstars-1].e;
      s[ierr].n = s[Nstars-1].n;
      s[ierr].u = s[Nstars-1].u;
      sigmax[ierr] = sigmax[Nstars-1];
      sigmay[ierr] = sigmay[Nstars-1];
      strcpy(sname[ierr], sname[Nstars-1]);
      Nstars--;
    }

  return Nstars;
}

float chi2_func(float x[])
{
  double chi2, xstar, ystar;
  int i;

  if(Nstars<NPARMS+1)return 1.0e0;

  chi2 = 0.0e0;

  for(i=0;i<Nstars;i++)
    {
      starpos(i, &xstar, &ystar, x);

      chi2 += pow((xmeas[i] - xstar)/sigmax[i],2.0e0);
      chi2 += pow((ymeas[i] - ystar)/sigmay[i],2.0e0);
    }

  chi2 /= (double)(2*Nstars-NPARMS);

  if(chi2!=chi2)chi2=1.0e10;

  return (float)chi2;
}

double fit_func(int N, double param[], float (*funk)(float []))
{
  int i, j, imin, nfunc;
  float *x, *y, **p;
  double fval;

  // Allocate memory

  x = (float *)vector(1,N);
  y = (float *)vector(1,N+1);
  p = (float **)matrix(1,N+1,1,N);

  // Set initial simplex values from the passed values of the parameters

  for(i=1;i<=N+1;i++)
    {
      for(j=1;j<=N;j++)
	{
	  x[j] = p[i][j] = (float)(param[j-1]*(1.0e0+(0.5e0-drand48())/10000.0e0));
	}
      y[i] = funk(x);
    }

  // Use amoeba to minimize

  nfunc = N+1; // Number of function evaluations

  amoeba(p, y, N, 1.0e-14, funk, &nfunc);

  // Find the minimum vertex

  imin = 1; for(i=2;i<=N+1;i++)if(y[i]<y[imin])imin = i;
  
  fval = (double)y[imin];

  // Set the parameters to the minimum values

  for(i=1;i<=N;i++)param[i-1] = (double)p[imin][i];

  // Free allocated memory

  free_matrix(p,1,N+1,1,N);
  free_vector(y,1,N+1);
  free_vector(x,1,N);

  // Return the minimum function value

  return fval;
}

void starpos(int istar, double *xstar, double *ystar, float x[])
{
  VECTOR m, xp, yp;
  double theta, phi, psi, x0, y0, D0;
  
  theta = (double)x[1];
  phi   = (double)x[2];
  if(NPARMS>2)
    D0    = (double)x[3];
  else
    D0    = xglobal[3];

  if(NPARMS>3)
    psi   = (double)x[4];
  else
    psi    = xglobal[4];

  if(NPARMS>4)
    x0    = (double)x[5];
  else
    x0    = xglobal[5];

  if(NPARMS>5)
    y0    = (double)x[6];
  else
    y0    = xglobal[6];

  if(psi!=psi)
    {
      fprintf(stderr,"x x ");
      fprintf(stderr,"%.3lf ", R2D*phi);
      fprintf(stderr,"%.3lf ", R2D*theta);
      fprintf(stderr,"%.3lf ", R2D*psi);
      fprintf(stderr,"%.3lf ", x0);
      fprintf(stderr,"%.3lf ", y0);
      fprintf(stderr,"%.3lf\n", D0);
      exit(-1);
    }

  m.e = cos(theta)*cos(phi);
  m.n = cos(theta)*sin(phi);
  m.u = sin(theta);
  
  xp.e = cos(psi)*sin(phi) - sin(psi)*sin(theta)*cos(phi);
  xp.n = -(cos(psi)*cos(phi) + sin(psi)*sin(theta)*sin(phi));
  xp.u = sin(psi)*cos(theta);
  
  yp.e = -(sin(psi)*sin(phi) + cos(psi)*sin(theta)*cos(phi));
  yp.n = sin(psi)*cos(phi) - cos(psi)*sin(theta)*sin(phi);
  yp.u = cos(psi)*cos(theta);
  
  *xstar = D0*dot(s[istar],xp)/dot(s[istar],m)+x0;
  *ystar = D0*dot(s[istar],yp)/dot(s[istar],m)+y0;

  return;
}
