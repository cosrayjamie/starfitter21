#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "eph_manager.h"
#include "novas.h"
#include "julian.h"

#define D2R (M_PI/180.0e0)
#define R2D (180.0e0/M_PI)
#define NSITES 5

typedef struct VECTOR
{
  double azi, elev;
  double e, n, u;
}VECTOR;

int main( int argc, char *argv[]);
double get_double(char buffer[], int ibeg, int iend);
int get_int(char buffer[], int ibeg, int iend);
int get_twilight(int site, int year, int month, int day, double *twilight_begin,  double *twilight_end);
int getsao(int HD, int *rSAO);

const double longitude[NSITES]   = {-112.908732522222e0, -112.99429273849e0, -112.71180369861e0, -113.12143272060e0, -112.983527e0};
const double latitude[NSITES]    = {39.2969179361111e0, 39.47294139643e0, 39.18834810787e0, 39.20792991505e0, 39.451425e0};
const double height[NSITES]      = {1370.046e0, 1589.193e0, 1398.621e0, 1546.650e0, 1550.93e0};

int main( int argc, char *argv[])
{
  FILE *pp;
  char buffer[1024], command[1024];
  int i;
  int iday, ihour, iminute;
  double dot, vmag;
  char tname[32];
  double maxdot[10];
  double maxmag[10];
  char maxstar[10][32];
  double maxelev[10];
  double maxazi[10];
  double jday, jd_start, jd_end;
  int site, year, month;
  double second;
  int HD, SAO;
  
  VECTOR mir[10] = {{92.083,58.825,0.0,0.0,0.0},
		    {87.954,50.368,0.0,0.0,0.0},
		    {105.493,69.065,0.0,0.0,0.0},
		    {99.002,64.431,0.0,0.0,0.0},
		    {158.371,77.585,0.0,0.0,0.0},
		    {158.046,77.619,0.0,0.0,0.0},
		    {219.722,64.998,0.0,0.0,0.0},
		    {212.368,69.803,0.0,0.0,0.0},
		    {231.333,50.612,0.0,0.0,0.0},
		    {227.047,59.115,0.0,0.0,0.0}};

  VECTOR star;

  for(i=0;i<10;i++)
    {
      mir[i].u = sin(mir[i].elev*D2R);
      mir[i].n = cos(mir[i].elev*D2R)*cos(mir[i].azi*D2R);
      mir[i].e = cos(mir[i].elev*D2R)*sin(mir[i].azi*D2R);
    }

  if(argc != 5)
    {
      fprintf(stderr,"%s site yyyy mm dd\7\7\7\\n\n", argv[0]);
      fprintf(stderr,"site   = Site number, CLF=0, MD=1, BR=2, LR=3, Laser=4\n");
      fprintf(stderr,"yyyy   = year\n");
      fprintf(stderr,"mm     = month\n");
      fprintf(stderr,"dd     = day\n");
      fprintf(stderr,"Version = 1.00\n");
      exit(-1);
    }

  site  = atoi(argv[1]);
  year  = atoi(argv[2]);
  month = atoi(argv[3]);
  iday   = atoi(argv[4]);

  if((iday<1)||(iday>31))exit(-1);
  if(year<2007)exit(-1);

  get_twilight(site, year, month, iday, &jd_start,  &jd_end);

  jdate(jd_start, &year, &month, &iday, &ihour, &iminute, &second);

  printf("Astronomical twilight begins: %d/%.2d/%.4d %.2d:%.2d:%.3lf\n", 
	 month, iday, year, ihour, iminute, second);

  for(jday=jd_start;jday<jd_end;jday += 6.0e0/8640.0e0)
    {
      jdate(jday, &year, &month, &iday, &ihour, &iminute, &second);

      for(i=0;i<10;i++)
	{
	  maxstar[i][0] = '\0';
	  maxdot[i] = 0.0e0;
	  maxmag[i] = 30.0e0;
	}
      sprintf(command,"./test_stars %d %d %d %d %d %d 0 20.0 840.0", site, year, month, iday, ihour, iminute);
	    
      pp = popen(command,"r");
	    
      while(fgets(buffer,1023,pp)!=NULL)
	{
	  if(sscanf(buffer,"%s %lf %lf %lf", tname, &star.elev, &star.azi, &vmag)!=4)continue;
		
	  star.u = sin(star.elev*D2R);
	  star.n = cos(star.elev*D2R)*cos(star.azi*D2R);
	  star.e = cos(star.elev*D2R)*sin(star.azi*D2R);
		
	  for(i=0;i<10;i++)
	    {
	      dot = star.e*mir[i].e + star.n*mir[i].n + star.u*mir[i].u;
		    
	      if(dot>maxdot[i])
		{
		  maxdot[i] = dot;
		  maxmag[i] = vmag;
		  strncpy(maxstar[i],tname,31);
		  maxelev[i] = star.elev;
		  maxazi[i] = star.azi;
		}
	    }
	}
      pclose(pp);
	    
      for(i=0;i<10;i++)
	{
	  if(maxdot[i]<cos(D2R/2.0e0))continue;
	  if(sscanf(maxstar[i],"HD%d", &HD)!=1)continue;
	  if(getsao(HD, &SAO)!=0)continue;

	  printf("%d/%d/%d %d:%.2d,m%.2d,SAO%.6d,%.3lf,%.3lf,%.2lf\n", 
		 month, iday, year, ihour, iminute,
		 i+15, SAO, maxazi[i], maxelev[i], maxmag[i]);
	}
    }

  jdate(jd_end, &year, &month, &iday, &ihour, &iminute, &second);

  printf("Astronomical twilight ends: %d/%.2d/%.4d %.2d:%.2d:%.3lf\n", 
	 month, iday, year, ihour, iminute, second);

  return 0;
}


int get_twilight(int site, int year, int month, int day, double *twilight_begin,  double *twilight_end)
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

  double zen1, zen2, jday;
  double jdmin, jdmax;
  
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
  
  jd_utc = julian_date (year, month, day, 0);
    
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
      
  i = 10;  // Sun

  // Look for beginning of twilight
      
  jd_utc = julian_date (year, month, day, 0);
  jd_tt = jd_utc + ((double)leap_secs + 32.184e0)/86400.0e0;
  jd_ut1 = jd_utc + ut1_utc/86400.0e0;
  delta_t = 32.184e0 + leap_secs - ut1_utc;
  if ((error = topo_planet (jd_tt, &planet[i], delta_t, &geo_loc, accuracy, &rat, &dect, &dist)) != 0)
    {
      fprintf(stderr, "Error %d from topo_planet.", error);
      exit(-1);
    }
  equ2hor (jd_ut1, delta_t, accuracy, x_pole, y_pole, &geo_loc, rat, dect, 2, &zen2, &az, &rar, &decr);

  do
    {
      do
	{
	  jd_utc += 30.0e0/86400.0e0;
	  zen1 = zen2;
	  //fprintf(stderr,"jd_utc = %lf\n", jd_utc);
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
	  // fprintf(stderr,"%.6lf %.6lf\r", (zen1-108.0e0), (zen2-108.0e0));
	}
      while((zen1-108.0e0)*(zen2-108.0e0)>0.0e0); // Cross twilight threshold
    }
  while(zen1 > 108.0e0); // previous zenith was below zenith threshold (beginning twilight)

  // Binary search for moment of beginning twilight

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
      //fprintf(stderr,"%.6lf %.6lf\r", (zen1-108.0e0), (zd-108.0e0));
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

  *twilight_begin = ((jdmin+jdmax)/2.0e0-2440000.0e0);

  // Look for ending of twilight

  jd_utc = *twilight_begin + 5.0e0/86400.0e0 + 2440000.0e0;
  jd_tt = jd_utc + ((double)leap_secs + 32.184e0)/86400.0e0;
  jd_ut1 = jd_utc + ut1_utc/86400.0e0;
  delta_t = 32.184e0 + leap_secs - ut1_utc;
  if ((error = topo_planet (jd_tt, &planet[i], delta_t, &geo_loc, accuracy, &rat, &dect, &dist)) != 0)
    {
      fprintf(stderr, "Error %d from topo_planet.", error);
      exit(-1);
    }
  equ2hor (jd_ut1, delta_t, accuracy, x_pole, y_pole, &geo_loc, rat, dect, 2, &zen2, &az, &rar, &decr);

  do
    {
      do
	{
	  zen1 = zen2;
	  jd_utc += 30.0e0/86400.0e0;
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
      while((zen1-108.0e0)*(zen2-108.0e0)>0.0e0); // Cross twilight threshold
    }
  while(zen1 < 108.0e0); // previous zenith was above zenith threshold (ending twilight)

  // Binary search for moment of ending twilight

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
      // fprintf(stderr,"%.6lf %.6lf\r", (zen1-108.0e0), (zd-108.0e0));
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

  *twilight_end = ((jdmin+jdmax)/2.0e0-2440000.0e0);

  ephem_close();		/* remove this line for use with solsys version 2 */
      
  return 0;
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

int getsao(int HD, int *rSAO)
{
  FILE *fp, *pp;
  char buffer[4096], command[2048], *stmp;
  int nHD[10000];
  int nSAO[10000];
  int mHD = 0;
  int year0;
  int ra_hh, ra_mm;
  double ra_ss;
  char dec_sgn;
  int dec_dd, dec_mm;
  double dec_ss;
  double ra_pm, dec_pm;
  double parallax;
  double rv;
  double vmag;
  int flag;
  int SAO;
  int i;

  for(i=0;i<mHD;i++)
    {
      if(nHD[i]==HD)
	{
	  *rSAO = nSAO[i];
	  if(nSAO[i] == (-1))return (-1);
	  return 0;
	}
    }
  nHD[mHD] = HD;
  nSAO[mHD++] = (-1);

  sprintf(command,"wget -O - \'http://simbad.u-strasbg.fr/simbad/sim-id?output.format=ASCII&output.max=1&Ident=hd%.6d\'", HD);

  pp = popen(command,"r");
  
  //2000 06 45 8.904 -16 42 57.960 -55.3 -120.5 -8 0.375 -1.46 HD048915
  
  flag=0;
  while(fgets(buffer,2047,pp)!=NULL)
    {
      if(flag<1)
	{
	  if(sscanf(buffer," Coordinates%*cICRS,ep=J%d,eq=%*d%*c: %d %d %lf -%d %d %lf",
		    &year0, &ra_hh, &ra_mm, &ra_ss, &dec_dd, &dec_mm, &dec_ss) == 7){dec_sgn='-'; dec_dd=abs(dec_dd); flag=1; continue;}
	  if(sscanf(buffer," Coordinates%*cICRS,ep=J%d,eq=%*d%*c: %d %d %lf +%d %d %lf",
		    &year0, &ra_hh, &ra_mm, &ra_ss, &dec_dd, &dec_mm, &dec_ss) == 7){dec_sgn='+'; dec_dd=abs(dec_dd); flag=1; continue;}
	  if(sscanf(buffer," Coordinates%*cICRS,ep=J%d,eq=%*d%*c: %d %d %lf %d %d %lf",
		    &year0, &ra_hh, &ra_mm, &ra_ss, &dec_dd, &dec_mm, &dec_ss) == 7)
	    {
	      if(dec_dd<0)
		dec_sgn='-';
	      else
		dec_sgn='+';
 
	      dec_dd=abs(dec_dd); 

	      flag=1; 
	    }
	  continue;
	}

      if(flag<2)
	{
	  if(strstr(buffer,"Proper motions:")==NULL)continue;
	  if(sscanf(buffer," Proper motions: %lf %lf", &ra_pm, &dec_pm) == 2)
	    {
	      flag=2;
	    }
	  else
	    {
	      ra_pm=dec_pm=0.0e0;
	      flag=2;
	    }
	  continue;
	}

      if(flag<3)
	{
	  if(strstr(buffer,"Parallax:")==NULL)continue;
	  if(sscanf(buffer," Parallax: %lf", &parallax)==1)
	    {
	      flag=3;
	    }
	  else
	    {
	      parallax = 0.0e0;
	      flag=3;
	    }
	  continue;
	}

      if(flag<4)
	{
	  if(strstr(buffer,"Radial Velocity")==NULL)continue;
	  if(sscanf(buffer," Radial Velocity: %lf", &rv)==1)
	    {
	      flag=4;
	    }
	  else
	    {
	      rv = 0.0e0;
	      flag=4;
	    }
	  continue;
	}
 

      if(flag<5)
	{
	  if(strstr(buffer,"Flux B ")==NULL)continue;
	  if(sscanf(buffer," Flux B : %lf", &vmag)==1){flag=5; continue;}
	}

      if(flag<6)
	{
	  if(strstr(buffer,"Flux V ")==NULL)continue;
	  if(sscanf(buffer," Flux V : %lf", &vmag)==1){flag=6; continue;}
	}

      if(flag<7)
	{
	  if((stmp = strstr(buffer,"SAO "))==NULL)continue;
	  if(sscanf(stmp," SAO %d", &SAO)==1)
	    {
	      flag=7;
	    }
	  break;
	}
    }

  pclose(pp);

  if(flag==7)
    {
      *rSAO = nSAO[mHD-1] = SAO;
      return 0;
    }

  return (-1);
}
