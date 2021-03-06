#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define sgn(A) (((double)A)/fabs((double)A))

int main( void);

typedef struct STAR
{
  int year;
  int ra_hh, ra_mm;
  double ra_ss;
  double dec_sgn;
  int dec_dd, dec_mm;
  double dec_ss;
  double ra_pm, dec_pm;
  double rv, parallax, vmag;
  int HD;
}STAR; 

int main( void)
{
  int i, N1, N2;
  FILE *fp1, *fp2;
  char buffer[1024];
  static STAR s1[10000], s2[10000];

  double v1, v2;
  char c1, c2;

  if((fp1 = fopen("bsc5.cat","r"))==NULL)exit(-1);
  if((fp2 = fopen("my_bsc5.cat","r"))==NULL)exit(-1);

  N1=N2=0;
  while(fgets(buffer,1023,fp1)!=NULL)
    {
      if(strstr(buffer," -0 ")!=NULL) // Handle -0 degree declination case
	{
	  s1[N1].dec_sgn = (-1.0e0);
	}
      else
	{
	  s1[N1].dec_sgn = (+1.0e0);
	}

      if(sscanf(buffer," %d %d %d %lf %d %d %lf %lf %lf %lf %lf %lf HD%d", 
		&s1[N1].year, 
		&s1[N1].ra_hh, &s1[N1].ra_mm, &s1[N1].ra_ss,
		&s1[N1].dec_dd, &s1[N1].dec_mm, &s1[N1].dec_ss,
		&s1[N1].ra_pm, &s1[N1].dec_pm,
		&s1[N1].rv, &s1[N1].parallax, &s1[N1].vmag, &s1[N1].HD) == 13)
	{
	  if(s1[N1].dec_dd<0)
	    {
	      s1[N1].dec_sgn = (-1.0e0);
	      s1[N1].dec_dd  = -s1[N1].dec_dd;
	    }
	  N1++;
	}
    }
  fclose(fp1);
  
  while(fgets(buffer,1023,fp2)!=NULL)
    {
      if(strstr(buffer," -0 ")!=NULL) // Handle -0 degree declination case
	{
	  s2[N2].dec_sgn = (-1.0e0);
	}
      else
	{
	  s2[N2].dec_sgn = (+1.0e0);
	}
      
      if(sscanf(buffer," %d %d %d %lf %d %d %lf %lf %lf %lf %lf %lf HD%d", 
		&s2[N2].year, 
		&s2[N2].ra_hh, &s2[N2].ra_mm, &s2[N2].ra_ss,
		&s2[N2].dec_dd, &s2[N2].dec_mm, &s2[N2].dec_ss,
		&s2[N2].ra_pm, &s2[N2].dec_pm,
		&s2[N2].rv, &s2[N2].parallax, &s2[N2].vmag, &s2[N2].HD) == 13)
	{
	  if(s2[N2].dec_dd<0)
	    {
	      s2[N2].dec_sgn = (-1.0e0);
	      s2[N2].dec_dd  = -s2[N2].dec_dd;
	    }
	  N2++;
	}
    }
  fclose(fp2);
      
  if(N1!=9096){fprintf(stderr,"bsc5.cat missing entries %d/9096\n", N1); exit(-1);}
  if(N2!=9096){fprintf(stderr,"my_bsc5.cat missing entries %d/9096\n", N2); exit(-1);}

  for(i=0;i<N1;i++)
    {
      if(s1[i].HD != s2[i].HD){fprintf(stderr,"Missmatched entry HD%.6d <> HD%.6d\n", s1[i].HD, s2[i].HD); exit(-1);}
      if(s1[i].year != s2[i].year){fprintf(stderr,"Missmatched entry HD%.6d: (year) %.4d %.4d\n", s1[i].HD, s1[i].year, s2[i].year); exit(-1);}

      v1 = ((double)s1[i].ra_hh*60.0e0+(double)s1[i].ra_mm)*60.0e0+s1[i].ra_ss; 
      v2 = ((double)s2[i].ra_hh*60.0e0+(double)s2[i].ra_mm)*60.0e0+s2[i].ra_ss; 

      if(fabs(v1-v2)>15.0e0)
	{
	  fprintf(stderr,"Missmatched entry HD%.6d: (RA) %.2d:%.2d:%.5lf %.2d:%.2d:%.5lf\n", 
		  s1[i].HD, 
		  s1[i].ra_hh, s1[i].ra_mm, s1[i].ra_ss,
		  s2[i].ra_hh, s2[i].ra_mm, s2[i].ra_ss);
	}

      // Fix for sign error

      if((s1[i].dec_dd==0)&&(s1[i].dec_sgn<0.0e0))
	{
	  if((s2[i].dec_dd==0)&&(s2[i].dec_sgn>0.0e0))
	    {
	      s2[i].dec_sgn = (-1.0e0);
	    }
	}

      v1 = ((fabs((double)s1[i].dec_dd)*60.0e0+(double)s1[i].dec_mm)*60.0e0+s1[i].dec_ss)*s1[i].dec_sgn+90.0e0*3600.0e0; 
      v2 = ((fabs((double)s2[i].dec_dd)*60.0e0+(double)s2[i].dec_mm)*60.0e0+s2[i].dec_ss)*s2[i].dec_sgn+90.0e0*3600.0e0; 

      if(fabs(v1-v2)>15.0e0)
	{
	  if(s1[i].dec_sgn<0.0e0)
	    c1 = '-';
	  else
	    c1 = '+';

	  if(s2[i].dec_sgn<0.0e0)
	    c2 = '-';
	  else
	    c2 = '+';
	     
	  fprintf(stderr,"Missmatched entry HD%.6d: (DEC) %c%d:%.2d:%.5lf %c%d:%.2d:%.5lf\n", 
		  s1[i].HD, 
		  c1, s1[i].dec_dd, s1[i].dec_mm, s1[i].dec_ss,
		  c2, s2[i].dec_dd, s2[i].dec_mm, s2[i].dec_ss);
	}


      if(fabs(s1[i].ra_pm-s2[i].ra_pm)>fabs(s1[i].ra_pm/2.0e0)){fprintf(stderr,"Missmatched entry HD%.6d: (ra_pm) %.4lf %.4lf\n", s1[i].HD, s1[i].ra_pm, s2[i].ra_pm);}
      if(fabs(s1[i].dec_pm-s2[i].dec_pm)>fabs(s1[i].dec_pm/2.0e0)){fprintf(stderr,"Missmatched entry HD%.6d: (dec_pm) %.4lf %.4lf\n", s1[i].HD, s1[i].dec_pm, s2[i].dec_pm);}
      if(fabs(s1[i].rv-s2[i].rv)>fabs(s1[i].rv/2.0e0)){fprintf(stderr,"Missmatched entry HD%.6d: (rv) %.4lf %.4lf\n", s1[i].HD, s1[i].rv, s2[i].rv);}
      if(fabs(s1[i].parallax-s2[i].parallax)>fabs(s1[i].parallax/2.0e0)){fprintf(stderr,"Missmatched entry HD%.6d: (parallax) %.4lf %.4lf\n", s1[i].HD, s1[i].parallax, s2[i].parallax);}
      if(fabs(s1[i].vmag-s2[i].vmag)>fabs(s1[i].vmag/2.0e0)){fprintf(stderr,"Missmatched entry HD%.6d: (vmag) %.4lf %.4lf\n", s1[i].HD, s1[i].vmag, s2[i].vmag);}
      

      // Print out verified and corrected entries

      if(s2[i].dec_sgn<0.0e0)
	c2 = '-';
      else
	c2 = '+';
	     
      printf("%4d ", s2[i].year);
      printf("%.2d %.2d %lf ", s2[i].ra_hh, s2[i].ra_mm, s2[i].ra_ss);
      printf("%c%d %.2d %lf ", c2, s2[i].dec_dd, s2[i].dec_mm, s2[i].dec_ss);
      printf("%lf %lf ", s2[i].ra_pm, s2[i].dec_pm);
      printf("%lf ", s2[i].rv);
      printf("%lf ", s2[i].parallax);
      printf("%lf ", s2[i].vmag);
      printf("HD%.6d\n", s2[i].HD);
      fflush(stdout);
    }

  return 0;
}


