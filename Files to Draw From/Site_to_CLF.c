/*
    Program to compare the current mirror geometry fit parameters to the original values.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define RAD2DEG  (180.0e0/M_PI)
#define DEG2RAD  (M_PI/180.0e0)
#define NMIRS 25
#define NSITES 3
#define dot(A,B)  (((A).e)*((B).e)+((A).n)*((B).n)+((A).u)*((B).u))
#define mag(A)    sqrt(dot(A,A))

typedef struct GEOM
{
  double azi[NSITES][NMIRS];  // Cluster azimuth angle
  double elev[NSITES][NMIRS]; // Cluster elevation angle
  double rot[NSITES][NMIRS];  // Cluster rotation angle
  double mcd[NSITES][NMIRS];  // Mirror-cluster distances (meters)
  double Rmir[NSITES][NMIRS]; // Mirror radius of curvature (meters)
}GEOM;

typedef struct VECTOR
{
  double e, n ,u;
}VECTOR;

int main( void);
void set_rot(double rot[3][3], double lambda, double phi);


const double clf_longitude = -112.908732522222e0;
const double clf_latitude = 39.2969179361111e0;
const double clf_height = 1370.046e0;


int main( void)
{
  FILE *op;
  char buffer[1024];
  int mir, site;
  int i,j,k;

  static GEOM original, current;

  const int MAXMIR[NSITES] = { 24, 11, 11}; 
  const int MINMIR[NSITES] = { 1, 0, 0};
  
  const double longitude[NSITES]   = {-112.99429273849e0, -112.71180369861e0, -113.12143272060e0};
  const double latitude[NSITES]    = {39.47294139643e0, 39.18834810787e0, 39.20792991505e0};
  const double height[NSITES]      = {1589.193e0, 1398.621e0, 1546.650e0};

  double rot[3][3], rot_clf[3][3], rot_site[3][3];
  double enu[3], enup[3];
  double newazi, newelev;


  if((op=fopen("mirror_geometry.original","r"))==NULL)
    {
      fprintf(stderr,"Unable to open mirror_geometry.dat\7\7\n");
      exit(-1);
    }
  
  while(fgets(buffer,1023,op)!=NULL)
    {
      if(sscanf(buffer,"%d %d", &site, &mir) != 2)
        {
          fprintf(stderr,"Error reading mirror_geometry.original\7\7\n");
          exit(-1);
        }
      
      if(sscanf(buffer,"%*d %*d %lf %lf %lf %lf %lf", 
		&original.azi[site][mir], &original.elev[site][mir], &original.rot[site][mir], 
		&original.mcd[site][mir], &original.Rmir[site][mir])!=5)
        {
          fprintf(stderr,"Error reading mirror_geometry.original\7\7\n");
          exit(-1);
        }
    }

  fclose(op);

  set_rot(rot_clf, clf_longitude, clf_latitude);

  for(site=0;site<NSITES;site++)
    {
      set_rot(rot_site, longitude[site], latitude[site]);

      // Compute rotation matrix from SITE to CLF coordinates

      for(i=0;i<3;i++)
	{
	  for(j=0;j<3;j++)
	    {
	      rot[i][j] = 0.0e0;
	      for(k=0;k<3;k++)
		{
		  rot[i][j] += rot_site[k][i]*rot_clf[k][j];
		}
	    }
	}

      for(mir=MINMIR[site];mir<=MAXMIR[site];mir++)
	{
	  // Compute east, north, up

	  enu[0] = cos(original.elev[site][mir]*DEG2RAD)*cos(original.azi[site][mir]*DEG2RAD);
	  enu[1] = cos(original.elev[site][mir]*DEG2RAD)*sin(original.azi[site][mir]*DEG2RAD);
	  enu[2] = sin(original.elev[site][mir]*DEG2RAD);

	  // Rotate to CLF coordinates

	  for(i=0;i<3;i++)
	    {
	      enup[i] = 0.0e0;
	      for(j=0;j<3;j++)
		{
		  enup[i] += rot[j][i]*enu[j];
		}
	    }

	  // Compute rotate elevation angle
	  
	  newelev = RAD2DEG*asin(enup[2]/sqrt(enup[0]*enup[0]+enup[1]*enup[1]+enup[2]*enup[2]));
	  newazi  = RAD2DEG*atan2(enup[1],enup[0]);
	  if(newazi<0.0e0)newazi += 360.0e0;

	  printf("%d %d %.6lf %.6lf %.3lf %.3lf %.3lf\n", 
		 site, mir,
		 newazi,
		 newelev,
		 0.0e0,
		 original.mcd[site][mir],
		 original.Rmir[site][mir]);
	  fflush(stdout);
	}
    }

  return 0;
}


void set_rot(double rot[3][3], double lambda, double phi)
{
  lambda *= DEG2RAD;
  phi *= DEG2RAD;

  rot[0][0] = -sin(lambda);
  rot[0][1] = -sin(phi)*cos(lambda);
  rot[0][2] = cos(phi)*cos(lambda);
  rot[1][0] = cos(lambda);
  rot[1][1] = -sin(phi)*sin(lambda);
  rot[1][2] = cos(phi)*sin(lambda);
  rot[2][0] = 0.0e0;
  rot[2][1] = cos(phi);
  rot[2][2] = sin(phi);

  return;
}

