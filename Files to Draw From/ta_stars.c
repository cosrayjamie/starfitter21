#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "eph_manager.h"
#include "novas.h"

#define CONSERVATIVE

#define RAD2DEG  (180.0e0/M_PI)
#define DEG2RAD  (M_PI/180.0e0)
#define NSTAR 9096
#define NMIRS 29
#define NSITES 4
#define dot(A,B)  (((A).e)*((B).e)+((A).n)*((B).n)+((A).u)*((B).u))
#define mag(A)    sqrt(dot(A,A))

typedef struct VECTOR
{
  double e, n ,u;
}VECTOR;

int main (int argc, char *argv[]);
double get_double(char buffer[], int ibeg, int iend);
int get_int(char buffer[], int ibeg, int iend);
int mirproj(int site, int mir, double star_elev, double star_azi, double *star_x, double *star_y);

const int MAXMIR[NSITES] = { 28, 11, 11, 8}; 
const int MINMIR[NSITES] = { 1, 0, 0, 1};
 
double azi[NSITES][NMIRS];  // Cluster azimuth angle
double elev[NSITES][NMIRS]; // Cluster elevation angle
double rot[NSITES][NMIRS];  // Cluster rotation angle
double mcd[NSITES][NMIRS];  // Mirror-cluster distances (meters)
double Rmir[NSITES][NMIRS]; // Mirror radius of curvature (meters)

const double longitude[NSITES]   = {-112.99429273849e0, -112.71180369861e0, -113.12143272060e0, -112.71180369861e0};
const double latitude[NSITES]    = {39.47294139643e0, 39.18834810787e0, 39.20792991505e0, 39.18834810787e0};
const double height[NSITES]      = {1589.193e0, 1398.621e0, 1546.650e0, 1398.621e0};

const double clf_longitude = -112.908732522222e0;
const double clf_latitude = 39.2969179361111e0;
const double clf_height = 1370.046e0;

const double CLUX[NSITES] = {15.0e0, 22.0e0, 22.0e0, 15.0e0};
const double CLUY[NSITES] = {15.0e0, 22.0e0, 22.0e0, 15.0e0};


int main (int argc, char *argv[])
{
  const int accuracy = 0;  // 0 is for highest accuracy
  FILE *pp, *fp, *op;
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

  double pmag[12] = {20.0e0,
		     -2.45e0,
		     -4.89e0,
		     0.0e0,
		     -2.91e0,
		     -2.94e0,
		     -0.49e0,
		     5.32e0,
		     7.78e0,
		     13.65e0,
		     -26.74e0,
		     -2.50e0};
  int mir;
  int year = 2011;
  int month = 4;
  int day = 24;
  int hour0 = 11;
  int minute = 37;
  double seconds = 5.322;
  double MJD;
  int leap_secs = 33;

  int error = 0;
  short int de_num = 0;

  double hour = 10.605;
  double ut1_utc = -0.387845;
  double tai_utc = 34.0e0;

  double x_pole = -0.002;
  double y_pole = +0.529;

  double jd_beg, jd_end, jd_utc, jd_tt, jd_ut1, jd_tdb, delta_t, ra,
    dec, dis, rat, dect, dist, zd, az, rar, decr, gast, last, theta,
    jd[2], pos[3], vel[3], pose[3], elon, elat, r, lon_rad, lat_rad,
    sin_lon, cos_lon, sin_lat, cos_lat, vter[3], vcel[3];

  on_surface geo_loc;

  observer obs_loc;

  static cat_entry star[NSTAR];
  cat_entry dummy_star;
  double vmag[NSTAR];

  object planet[12];

  sky_pos t_place;
  int i, N = 9096;
  char buffer[2048];

  double temperature = 20.0e0;
  double pressure    = 840.0e0;

  int istar;
  double altitude, azimuth;
  int getP, getV, getAA;

  double jd0;
  int year0, month0, day0;

  int  ra_hh, ra_mm;
  double ra_ss;
  int dec_dd, dec_mm;
  double decval;
  double dec_ss;
  double ra_pm, dec_pm, rv, parallax;
  char sname[128];
  int site;
  double star_x, star_y;

  // Load mirror geometry 

  if((op=fopen("mirror_geometry.dat","r"))==NULL)
    {
      fprintf(stderr,"Unable to open mirror_geometry.dat\7\7\n");
      exit(-1);
    }

  while(fgets(buffer,1023,op)!=NULL)
    {
      if(sscanf(buffer,"%d %d", &site, &mir) != 2)
	{
	  fprintf(stderr,"Error reading mirror_geometry.dat\7\7\n");
	  exit(-1);
	}

      if(sscanf(buffer,"%*d %*d %lf %lf %lf %lf %lf", &azi[site][mir], &elev[site][mir], 
		&rot[site][mir], &mcd[site][mir], &Rmir[site][mir])!=5)
	{
	  fprintf(stderr,"Error reading mirror_geometry.dat\7\7\n");
	  exit(-1);
	}
    }

  fclose(op);


  for(site=0;site<NSITES;site++)
    {
      for(mir=0;mir<NMIRS;mir++)
	{
	  azi[site][mir] = 90.0e0 - azi[site][mir];
	  if(azi[site][mir] < 0.0e0)azi[site][mir] += 360.0e0;
	}
    }

 if(argc<10)
    {
      fprintf(stderr,"%s site mir yyyy mm dd hh mm ss.sss tt.ttt pp.ppp\7\7\7\\n\n", argv[0]);
      fprintf(stderr,"site   = Site number, MD/TALE/TAx4=0, BR=1, LR=2, BRTAx4=3\n");
      fprintf(stderr,"mir    = Mirror number\n");
      fprintf(stderr,"yyyy   = year\n");
      fprintf(stderr,"mm     = month\n");
      fprintf(stderr,"dd     = day\n");
      fprintf(stderr,"hh     = hour\n");
      fprintf(stderr,"mm     = minute\n");
      fprintf(stderr,"ss.sss = second (decimal)\n");
      fprintf(stderr,"tt.ttt = temperature (degs C)\n");
      fprintf(stderr,"pp.ppp = pressure (millibars)\n");
      fprintf(stderr,"Version = 1.00\n");
      exit(-1);
    }


  site = atoi(argv[1]);
  if((site<0)||(site>NSITES))exit(-1);

  mir = atoi(argv[2]);
  if((mir<MINMIR[site])||(mir>MAXMIR[site]))exit(-1);

  year = atoi(argv[3]);
  if((year<1992)||(year>2030))exit(-1);

  month = atoi(argv[4]);
  if((month<1)||(month>12))exit(-1);

  day = atoi(argv[5]);
  if((day<1)||(day>31))exit(-1);

  hour0 = atoi(argv[6]);
  if((hour0<0)||(hour0>23))exit(-1);
  
  minute = atoi(argv[7]);
  if((minute<0)||(minute>59))exit(-1);
  
  seconds = atof(argv[8]);
  if((seconds<0.0e0)||(seconds>=60.0e0))exit(-1);
  
  temperature = atof(argv[9]);
  if((temperature < -50.0e0)||(temperature > 50.0e0))exit(-1);
  
  pressure = atof(argv[10]);
  if((pressure < 800.0e0)||(pressure > 870.0e0))exit(-1);

  hour = (double)hour0 + ((double)minute + seconds/60.0e0)/60.0e0;

  jd_utc = julian_date (year, month, day, hour);

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

  jd_utc = julian_date (year, month, day, hour);
  jd_tt = jd_utc + ((double)leap_secs + 32.184e0)/86400.0e0;
  jd_ut1 = jd_utc + ut1_utc/86400.0e0;
  delta_t = 32.184e0 + leap_secs - ut1_utc;
  jd_tdb = jd_tt; // Approximation good to 0.0017 seconds.

#ifdef SITEORIGIN
  make_on_surface (latitude[site], longitude[site], height[site], temperature, pressure, &geo_loc);
  make_observer_on_surface (latitude[site], longitude[site], height[site], temperature,  pressure, &obs_loc);
#else
  make_on_surface (clf_latitude, clf_longitude, clf_height, temperature, pressure, &geo_loc);
  make_observer_on_surface (clf_latitude, clf_longitude, clf_height, temperature,  pressure, &obs_loc);
#endif

  if((fp=fopen("bsc5.cat","r"))==NULL)exit(-1);

  i = 0;
  while((fgets(buffer,2047,fp)!=NULL)&&(i<NSTAR))
    {
      if(sscanf(buffer," %d %d %d %lf %d %d %lf %lf %lf %lf%lf %lf %s",
		&year0, &ra_hh, &ra_mm, &ra_ss, &dec_dd, &dec_mm, &dec_ss,
		&ra_pm, &dec_pm, &rv, &parallax, &vmag[i], sname) != 13)continue;

   
      decval  = dec_ss;
      decval /= 60.0e0;
      decval += (double)dec_mm;
      decval /= 60.0e0;
      decval += fabs((double)dec_dd);

      if(dec_dd != 0)
	{
	  decval *= (double)dec_dd/fabs((double)dec_dd);
	}
      else
	{
	  if(strstr(buffer," -0 ")!=NULL) // Handle -0 degree declination case
	    {
	      decval = (-decval);
	    }
	}
      
      if(make_cat_entry (sname, "BSC5", (long int)i, 
			 (double)ra_hh+((double)ra_mm + ra_ss/60.0e0)/60.0e0,
			 decval,
			 ra_pm*10.0e0, dec_pm*10.0e0,
			 parallax*1.0e4, 
			 rv, &star[i]) != 0)exit(-1);
      i++;
    }
  fclose(fp);
  
  if(i != NSTAR)exit(-1);

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

  for(i=0;i<NSTAR;i++)
    {
      if(vmag[i]>5.9e0)continue;
      if ((error = topo_star (jd_tt, delta_t, &star[i], &geo_loc, accuracy, &rat, &dect)) != 0)
	{
	  fprintf(stderr, "Error %d from topo_star.\n", error);
	  exit(-1);
	}

      equ2hor (jd_ut1, delta_t, accuracy, x_pole, y_pole, &geo_loc, rat, dect, 2, &zd, &az, &rar, &decr);

      if(fabs(elev[site][mir] - (90.0e0-zd)) < 20.0e0)
	{
	  if(fabs(azi[site][mir] - az) < 30.0e0)
	    {
	      if(mirproj(site, mir, 90.0e0-zd, az, &star_x, &star_y)==0)
		{
		  if((fabs(star_x)<CLUX[site])&&(fabs(star_y)<CLUY[site]))
		    {
#ifdef CONSERVATIVE
			if(vmag[i]>5.47)continue;
#endif
			printf("%s %.3lf %.3lf %.5lf %.5lf %.2lf\n", star[i].starname, 90.0e0-zd, az, star_x, star_y, vmag[i]);
		    }
		}
	    }
	}
    }

/*
   Apparent and topocentric place of the Planets
*/

  for(i=1;i<12;i++)
    {
      if(i==3)continue;
      if(i==10)continue;
      
      if ((error = topo_planet (jd_tt, &planet[i], delta_t, &geo_loc, accuracy, &rat, &dect, &dist)) != 0)
	{
	  fprintf(stderr, "Error %d from topo_planet.", error);
	  exit(-1);
	}

      equ2hor (jd_ut1, delta_t, accuracy, x_pole, y_pole, &geo_loc, rat, dect, 2, &zd, &az, &rar, &decr);
      
      if(fabs(elev[site][mir] - (90.0e0-zd)) < 20.0e0)
	{
	  if(fabs(azi[site][mir] - az) < 30.0e0)
	    {
	      if(mirproj(site, mir, 90.0e0-zd, az, &star_x, &star_y)==0)
		{
		  if((fabs(star_x)<1.01e0*CLUX[site])&&(fabs(star_y)<CLUY[site]))
		    {
		      printf("%s %.3lf %.3lf %.2lf %.2lf %.2lf\n", planet[i].name, 90.0e0-zd, az, star_x, star_y, pmag[i]);
		    }
		}
	    }
	}
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
      
/*
Program to compute Middle Drum nominal star positions on a mirror - SBT

*/

int mirproj(int site, int mir, double star_elev, double star_azi, double *star_x, double *star_y)
{
  double z0;
  VECTOR xhat, yhat, zhat, shat;

  if((site<0)||(site>NSITES))return (-1);
  if((mir<MINMIR[site])||(mir>MAXMIR[site]))return (-1);

  // Set the mirror optics distance. This is the value that should be used in mirror optics 
  // calculations (e.g. the value of D in X = D*tan(theta)). 
  
  z0 = (Rmir[site][mir] - mcd[site][mir])*100.0e0/2.54e0;  // Convert to inches
  
  //  star_azi = 90.0e0 - star_azi;
  //  if(star_azi < 0.0e0)star_azi += 360.0e0;


  zhat.e = cos(elev[site][mir]*DEG2RAD)*cos(azi[site][mir]*DEG2RAD);
  zhat.n = cos(elev[site][mir]*DEG2RAD)*sin(azi[site][mir]*DEG2RAD);
  zhat.u = sin(elev[site][mir]*DEG2RAD);
  
  xhat.e = cos(rot[site][mir]*DEG2RAD)*sin(azi[site][mir]*DEG2RAD) - sin(rot[site][mir]*DEG2RAD)*sin(elev[site][mir]*DEG2RAD)*cos(azi[site][mir]*DEG2RAD);
  xhat.n = -(cos(rot[site][mir]*DEG2RAD)*cos(azi[site][mir]*DEG2RAD) + sin(rot[site][mir]*DEG2RAD)*sin(elev[site][mir]*DEG2RAD)*sin(azi[site][mir]*DEG2RAD));
  xhat.u = sin(rot[site][mir]*DEG2RAD)*cos(elev[site][mir]*DEG2RAD);
  
  yhat.e = -(sin(rot[site][mir]*DEG2RAD)*sin(azi[site][mir]*DEG2RAD) + cos(rot[site][mir]*DEG2RAD)*sin(elev[site][mir]*DEG2RAD)*cos(azi[site][mir]*DEG2RAD));
  yhat.n = sin(rot[site][mir]*DEG2RAD)*cos(azi[site][mir]*DEG2RAD) - cos(rot[site][mir]*DEG2RAD)*sin(elev[site][mir]*DEG2RAD)*sin(azi[site][mir]*DEG2RAD);
  yhat.u = cos(rot[site][mir]*DEG2RAD)*cos(elev[site][mir]*DEG2RAD);
  
  shat.e = (cos(star_elev*DEG2RAD)*cos(star_azi*DEG2RAD));
  shat.n = (cos(star_elev*DEG2RAD)*sin(star_azi*DEG2RAD));
  shat.u = sin(star_elev*DEG2RAD);

  *star_x = z0*dot(shat,xhat)/dot(shat,zhat);
  *star_y = z0*dot(shat,yhat)/dot(shat,zhat);
  
  //  *star_x = z0*(shat.e*xhat.e+shat.n*xhat.n+shat.u*xhat.u);
  //  *star_y = z0*(shat.e*yhat.e+shat.n*yhat.n+shat.u*yhat.u);
         
  return 0;
}

