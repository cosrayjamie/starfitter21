/* 

  Star report 

  Routine to compare measured and predicted star positions given a mirror geometry

  20120807 - Stan Thomas
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "new_starfitter.h"
#include "nrutil.h"

void print_chi2(float x[]);

VECTOR s[NSTARS];
double xmeas[NSTARS], ymeas[NSTARS];
double sigmax[NSTARS], sigmay[NSTARS];
char sname[NSTARS][12];

const int MAXMIR[NSITES] = { 24, 11, 11}; 
const int MINMIR[NSITES] = { 1, 0, 0};

double azi[NSITES][NMIRS];  // Cluster azimuth angle
double elev[NSITES][NMIRS]; // Cluster elevation angle
double rot[NSITES][NMIRS];  // Cluster rotation angle
double mcd[NSITES][NMIRS];  // Mirror-cluster distances (meters)
double Rmir[NSITES][NMIRS]; // Mirror radius of curvature (meters)

int Nstars = 0;
int NPARMS = 6;
int NFIT = 0;

double xglobal[9];   // Global storage of x parameter vector

int site, mir;

int main(int argc, char *argv[])
{
  FILE *fp, *op;
  static double parms[10];  // Maximum number of parameters
  float x[10];
  int i;
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
      fprintf(stderr,"Usage: %s NFIT <mirror_geometry.dat> <*.stars.txt>\7\7\n", argv[0]);
      exit(-1);
    }

  NFIT = atoi(argv[1]);

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

      s[Nstars].e = cos(stheta)*cos(sphi);
      s[Nstars].n = cos(stheta)*sin(sphi);
      s[Nstars].u = sin(stheta);
      
      sigmax[Nstars] = 0.081e0;
      sigmay[Nstars] = 0.081e0;

      Nstars++;
    }

  fclose(fp);

  parms[0] = theta = elev[site][mir]*D2R;
  parms[1] = phi   = azi[site][mir]*D2R;
  parms[2] = D0 = (Rmir[site][mir] - mcd[site][mir])*100.0e0/2.54e0;
  parms[3] = psi   = rot[site][mir]*D2R;
  parms[4] = x0 = 0.0e0;
  parms[5] = y0 = 0.0e0; 

  for(i=0;i<6;i++)xglobal[i+1] = (float)parms[i]; // Save all parameters globally
  for(i=0;i<NPARMS;i++)x[i+1] = (float)parms[i]; // Save fit parameters

  print_chi2(x);

  return 0;
}

void print_chi2(float x[])
{
  double dist, xstar, ystar;
  int i;
  FILE *rp;

  if(Nstars<NPARMS+1)return;

  if((rp=fopen("star_report.dat","a"))==NULL)exit(-1);

  for(i=0;i<Nstars;i++)
    {
      starpos(i, &xstar, &ystar, x);
      dist  = pow((xmeas[i] - xstar),2.0e0);
      dist += pow((ymeas[i] - ystar),2.0e0);
      dist = sqrt(dist);
      fprintf(rp, "%d %d %d %d %.3lf %.3lf %.3lf %.3lf %lf\n", NFIT, site+1, mir, i+1, 25.4e0*xmeas[i], 25.4e0*ymeas[i], 25.4e0*xstar, 25.4e0*ystar, 25.4e0*dist);
    }

  fclose(rp);

  return;
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
