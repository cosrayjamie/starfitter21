#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( void);

int main( void)
{
  FILE *fp, *pp;
  char buffer[4096], command[2048], *stmp;
  int HD;
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

  if((fp=fopen("starnames.dat","r"))==NULL)exit(-1);

  while(fgets(buffer,2047,fp)!=NULL)
    {
      if(sscanf(buffer, "HD%d", &HD)!=1)exit(-1);

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
	  printf("HD%.6d ", HD);
	  printf("SAO%.6d ", SAO);
	  printf("J%4d ", year0);
	  printf("ra: %.2dh %.2d\' %.2lf\" ", ra_hh, ra_mm, ra_ss);
	  printf("dec: %c%d %.2d\' %.2lf\" ", dec_sgn, dec_dd, dec_mm, dec_ss);
	  printf("mag: %.2lf\n", vmag);
	  fflush(stdout);
	}
    }

  fclose(fp);

  return 0;
}
