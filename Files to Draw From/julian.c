#include <math.h>
#include <stdio.h>
#include "julian.h"

double julian( int year, int month, int day, int hour, int minute, double second)
{
  
/*****************************************************************************
!    The input time is assumed to be universal time.  Each input variable
!    is an integer.  The double precision Julian date returned has had 
!    subtracted from it to give room for better than millsecondond 
!
!    Here are some checks.
!    (1985,1,1,0,0,0,0) gives 6066.500000000  (cf. Corbato thesis).
!    (2000,1,1,12,0,0,0), gives 11545.0000000 (cf. The Astronomical Almanac).
!    Increments in hours, minutes, seconds, and millseconds give the right 
!    fractional day increments.
!    -- Paul Sommers 10/27/92
*****************************************************************************/

  int a,b, iyy, imm;
  double jd, time;

  if(year<200)year += 1900;
  
  if(month<=2)
  {
    year--;
    month += 12;
  }
  a   = year/100;
  b   = 2 - a + (a/4);
  jd  = floor(365.25E0*(double)year) + floor(30.6001E0*(double)(month+1)) + (double)(day + b);
  jd -= 719005.5E0;

  time  = second;
  time /= 60.0E0;
  time += (double)minute;
  time /= 60.0E0;
  time += (double)hour;
  time /= 24.0E0;

  jd  += time;

  return jd;
}

void jdate( double jd, int *year, int *month, int *day, int *hour, int *minute, double *second)
{
    double dj, f, z, a, b, c, d, e, alfa, dday, help, help1, help2;

    dj = jd + 0.5E0;
    dj +=2440000.0E0;

    f = modf(dj , &z);

    if(z < 2299161.0E0)
      a = z;
    else
    {
      alfa = floor((z - 1867216.25E0) / 36524.25E0);
      a = z + 1.0E0 + alfa - floor(alfa / 4.0E0);
    }
    b = a + 1524.0E0;
    c = floor((b - 122.1E0) / 365.25E0);
    d = floor(365.25E0 * c);
    e = floor((b - d) / 30.6001E0);
    dday = b - d - floor(30.6001E0 * e) + f;
    help = modf(dday, &help1);
    *day = (int)help1;
    help1 = help * 24.0E0;
    help = modf(help1, &help2);
    *hour = (int)help2;
    help1 = help * 60.0E0;
    help = modf(help1, &help2);
    *minute = (int)help2;
    *second = help * 60.0e0;
    if(e < 14.0E0)
      *month = (int)(e - 1.0E0);
    else
      *month = (int)(e - 13.0E0);
    if(*month > 2)
      *year = (int)(c - 4716.0E0);
    else
      *year = (int)(c - 4715.0E0);
    return;
}


void doy2md(int year, int doy, int *month, int *day)
{
  int table[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};
  int mnth, days;

  if(((year%4==0)&&(!(year%100==0)))||(year%400==0)) 
    table[2]=29;
  else 
    table[2]=28;
  
  days=0;
  for(mnth=1;mnth<=12;mnth++)
    {
      if((doy-days) <= table[mnth])break;
      days += table[mnth];
    }
  
  *day = doy - days;
  *month = mnth;

  return;
}

