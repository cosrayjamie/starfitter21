#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "eph_manager.h"
#include "novas.h"

#define NSITES   4

int main (int argc, char *argv[]);
double get_double(char buffer[], int ibeg, int iend);
int get_int(char buffer[], int ibeg, int iend);

const double longitude[NSITES]   = {-112.908732522222e0, -112.99429273849e0, -112.71180369861e0, -113.12143272060e0};
const double latitude[NSITES]    = {39.2969179361111e0, 39.47294139643e0, 39.18834810787e0, 39.20792991505e0};
const double height[NSITES]      = {1370.046e0, 1589.193e0, 1398.621e0, 1546.650e0};

int main (int argc, char *argv[])
{
  const int accuracy = 0;  // 0 is for highest accuracy
  FILE *pp;
  char *pname[12] = {"none",
		    "Mercury",
		    "Venus",
		    "Earth",
		    "Mars",
		    "Jupiter",
		    "Saturn",
		    "Uranus",
		    "Neptune",
		    "Pluto",
		    "Sun",
		    "Moon"};

  int year = 2011;
  int month = 4;
  int day = 24;
  double MJD;
  int leap_secs = 33;

  int error = 0;
  short int de_num = 0;

  double ut1_utc = -0.387845;
  double tai_utc = 34.0e0;

  double x_pole = -0.002;
  double y_pole = +0.529;

  double jd_beg, jd_end, jd_utc, jd_tt, jd_ut1, delta_t, rat, dect, dist, zd, az, rar, decr;

  on_surface geo_loc;
  observer obs_loc;

  cat_entry dummy_star;
  object planet[12];

  int i, getP;
  char buffer[2048];

  double temperature = 20.0e0;
  double pressure    = 840.0e0;

  double jd0;
  int year0, month0, day0;

  int site;

  double zen1, zen2, jday;
  double jdmin, jdmax;
  
  if(argc!=2)
    {
      fprintf(stderr,"%s site \7\7\7\\n\n", argv[0]);
      fprintf(stderr,"site   = Site number, CLF=0, MD=1, BR=2, LR=3\n");
      fprintf(stderr,"Version = 1.00\n");
      exit(-1);
    }


  site = atoi(argv[1]);
  if((site<0)||(site>=NSITES))exit(-1);


  // Set observer location and weather conditions
  
  make_on_surface (latitude[site], longitude[site], height[site], temperature, pressure, &geo_loc);
  make_observer_on_surface (latitude[site], longitude[site], height[site], temperature,  pressure, &obs_loc);
  
  //   Make structures of type 'object' for the planets, Sun, and Moon.
  
  make_cat_entry ("DUMMY", "xxx", 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, &dummy_star);
  
  for(i=1;i<12;i++)
    {
      if ((error = make_object (0, i, pname[i], &dummy_star, &planet[i])) != 0)
	{
	  printf ("Error %d from make_object (%s)\n", error, pname[i]);
	  exit(-1);
	}
    }
  
  /*
    Open the JPL binary ephemeris file, here named "JPLEPH".
    Remove this block for use with solsys version 2.
  */
  
  if ((error = ephem_open ("JPLEPH", &jd_beg, &jd_end, &de_num)) != 0)
    {
      if (error == 1)
	fprintf(stderr, "JPL ephemeris file not found.\n");
      else
	fprintf(stderr,"Error reading JPL ephemeris file header.\n");
      exit(-1);
    }
  


  //  jd_utc = julian_date (year, month, day, hour);

  for(jday = 2454218.0e0; jday < 2459397.0e0; jday += 0.1e0)
    {
      jd_utc = jday;

      if(jd_utc < 2414992.5e0)exit(-1); // Ephemeris limits
      if(jd_utc > 2469808.5e0)exit(-1); // Ephemeris limits

      // Obtain time parameters

      leap_secs = getP = 0;
      
      if(access("leapsec.dat",R_OK)!=0)
	{
	  if(system("wget -O leapsec.dat http://maia.usno.navy.mil/ser7/leapsec.dat")!=0)
	    {
	      fprintf(stderr," Failed to obtain leapsec.dat\n");
	      exit(-1);
	    }
	}
      
      if((pp=fopen("leapsec.dat","r"))!=NULL)
	{
	  while(fgets(buffer,2047,pp)!=NULL)
	    {
	      if(sscanf(buffer," %*d %*s %*d %*s %lf %*s %lf", &jd0, &tai_utc)!=2)continue;
	      if(jd0< jd_utc)leap_secs = (int)floor(tai_utc);
	    }
	  fclose(pp);
	}
      else
	{
	  fprintf(stderr," Failed to read leapsec.dat\n");
	  exit(-1);
	}
      
      //  printf("Leap Seconds = %d\n", leap_secs);
  
      if(access("finals2000A.data",R_OK)!=0)
	{
	  if(system("wget -O finals2000A.data http://maia.usno.navy.mil/ser7/finals2000A.data 2>/dev/null")!=0)
	    {
	      fprintf(stderr," Failed to obtain finals2000A.data\n");
	      exit(-1);
	    }
	}
      
      getP=0;
      if((pp=fopen("finals2000A.data","r"))!=NULL)
	{
	  while(fgets(buffer,2047,pp)!=NULL)
	    {
	      if(strlen(buffer)<176)continue;
	      
	      month0 = get_int(buffer, 3, 4);
	      if(month0!=month)continue;
	      
	      day0   = get_int(buffer, 5, 6);
	      if(day0!=day)continue;
	      
	      year0 = get_int(buffer, 1, 2);
	      MJD  = get_double(buffer, 8, 15);
	      
	      if((int)floor(MJD)<=51543)
		year0 += 1900;
	      else
		year0 += 2000;
	      if(year0!=year)continue;
	      
	      x_pole  = get_double(buffer, 19, 27);
	      y_pole  = get_double(buffer, 38, 46);
	      
	      ut1_utc = get_double(buffer, 59, 68);
	      
	      getP = 1;
	      break;
	    }
	  fclose(pp);
	}
      else
	{
	  fprintf(stderr," Failed to read finals2000A.data\n");
	  exit(-1);
	}

      // jd_utc = julian_date (year, month, day, hour);


      i = 10;  // Sun

      jd_utc = jday;
      jd_tt = jd_utc + ((double)leap_secs + 32.184e0)/86400.0e0;
      jd_ut1 = jd_utc + ut1_utc/86400.0e0;
      delta_t = 32.184e0 + leap_secs - ut1_utc;
      if ((error = topo_planet (jd_tt, &planet[i], delta_t, &geo_loc, accuracy, &rat, &dect, &dist)) != 0)
	{
	  fprintf(stderr, "Error %d from topo_planet.", error);
	  exit(-1);
	}
      equ2hor (jd_ut1, delta_t, accuracy, x_pole, y_pole, &geo_loc, rat, dect, 2, &zen1, &az, &rar, &decr);

      do
	{
	  jd_utc += 5.0e0*60.0e0/86400.0e0;
	  jday = jd_utc;
	  jd_tt = jd_utc + ((double)leap_secs + 32.184e0)/86400.0e0;
	  jd_ut1 = jd_utc + ut1_utc/86400.0e0;
	  delta_t = 32.184e0 + leap_secs - ut1_utc;
	  if ((error = topo_planet (jd_tt, &planet[i], delta_t, &geo_loc, accuracy, &rat, &dect, &dist)) != 0)
	    {
	      fprintf(stderr, "Error %d from topo_planet.", error);
	      exit(-1);
	    }
	  equ2hor (jd_ut1, delta_t, accuracy, x_pole, y_pole, &geo_loc, rat, dect, 2, &zen2, &az, &rar, &decr);
	  //	  fprintf(stderr,"%.6lf %.6lf\r", (zen1-108.0e0), (zen2-108.0e0));
	}
      while((zen1-108.0e0)*(zen2-108.0e0)>0.0e0);

      jdmin = jday;
      jdmax = jd_utc;

      do
	{
	  jd_utc = (jdmin + jdmax)/2.0e0;
	  jday = jd_utc;
	  jd_tt = jd_utc + ((double)leap_secs + 32.184e0)/86400.0e0;
	  jd_ut1 = jd_utc + ut1_utc/86400.0e0;
	  delta_t = 32.184e0 + leap_secs - ut1_utc;
	  if ((error = topo_planet (jd_tt, &planet[i], delta_t, &geo_loc, accuracy, &rat, &dect, &dist)) != 0)
	    {
	      fprintf(stderr, "Error %d from topo_planet.", error);
	      exit(-1);
	    }
	  equ2hor (jd_ut1, delta_t, accuracy, x_pole, y_pole, &geo_loc, rat, dect, 2, &zd, &az, &rar, &decr);
	  //	  fprintf(stderr,"%.6lf %.6lf\r", (zen1-108.0e0), (zd-108.0e0));
	  if((zen1-108.0e0)*(zd-108.0e0)>0.0e0)
	    {
	      zen1 = zd;
	      jdmin = jd_utc;
	    }
	  else
	    {
	      zen2 = zd;
	      jdmax = jd_utc;
	    }
	}
      while(fabs(jdmax-jdmin)*86400.0e0>0.1e0); // 0.1 seconds

      if(zen1-108.0e0 < 0.0e0)
	printf("%.12lf ", (jdmin+jdmax)/2.0e0-2440000.0e0);
      else
	printf("%.12lf\n", (jdmin+jdmax)/2.0e0-2440000.0e0);

      jday = jdmax;
    }

  ephem_close ();		/* remove this line for use with solsys version 2 */

  return (0);
}


double get_double(char buffer[], int ibeg, int iend)
{
  char *sub;
  int len;
  double result;
  
  ibeg--;
  iend--;
  len = strlen(buffer);
  if(ibeg<0)return 0.0e0;
  if(iend>=len)return 0.0e0;
  if(iend-ibeg<0)return 0.0e0;
  
  sub = strndup(&buffer[ibeg],iend-ibeg+1);
  
  result = atof(sub);
  free(sub);

  return result;
}

int get_int(char buffer[], int ibeg, int iend)
{
  char *sub;
  int len;
  int result;

  ibeg--;
  iend--;
  
  len = strlen(buffer);
  if(ibeg<0)return 0.0e0;
  if(iend>=len)return 0.0e0;
  if(iend-ibeg<0)return 0.0e0;
  
  sub = strndup(&buffer[ibeg],iend-ibeg+1);
  
  result = atoi(sub);
  free(sub);

  return result;
}
