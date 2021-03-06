#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PMAX 32
#define NMAX 1024

int main( void);
static int compare_doubles (const double * x, const double * y);

int main( void)
{
  FILE *pp, *fp;
  char buffer[2048], command[2048], fname[2048];
  int site, mir, nparms[PMAX], iboot[NMAX];
  int site0[NMAX], mir0[NMAX], nparms0[NMAX];
  double phi[NMAX], theta[NMAX], psi[NMAX], mcd[NMAX], roc[NMAX], nstars[NMAX], chi2[NMAX];
  int wc;
  int nc;
  char *sname[4] = {"Middle Drum","Black Rock","Long Ridge","Black Rock TAx4"};
  int ip, NP = 0;

  // Identify parameter settings from data file

  pp = popen("cat mirror_geometry.nparm.dat | awk \'{print $3}\' | sort -g | uniq","r");
  
  while(fgets(buffer,12,pp)!=NULL)
    {
      if(sscanf(buffer," %d", &nparms[NP])==1)NP++;
      if(NP>=PMAX)break;
    }

  pclose(pp);
      
  // Now process statistics for each set of parameter fits

  for(ip=0;ip<NP;ip++) // Parameter mask
    {
      // First see if we already have a stats file for this parameter mask - if so, clear it out.

      sprintf(fname,"mirror_geometry.%dparm.stats.csv", nparms[ip]);
      if((fp=fopen(fname,"w"))==NULL){fprintf(stderr,"FATAL ERROR - unable to create \"%s\"\n", fname); exit(-1);}
      fclose(fp);

      // Process all sites and mirrors for this parameter mask setting

      for(site=0;site<4;site++)  // Site
	{
	  for(mir=0;mir<29;mir++) // Mirror
	    {
	      if((site==0 || site==3) && (mir==0))continue; // MD and BR TAx4 has no mirror #00
	      if((site>0)&&(mir>11))break; // BR & LR have only 12 mirrors
	      
	      // First see how many records we have for this site, mirror, and parameter mask

	      sprintf(command,"grep \"%d %d %d \" ./mirror_geometry.nparm.dat | grep -v \" %d %d %d \" | wc -l", site, mir, nparms[ip], site, mir, nparms[ip]);
	      
	      if((pp = popen(command,"r"))==NULL)exit(-1);
	      if(fgets(buffer,2047,pp) == NULL)exit(-1);
	      if(sscanf(buffer," %d", &wc)!=1)exit(-1);
	      pclose(pp);
	      if(wc<900)
		{
		  fprintf(stderr,"Too few records for %s m%.2d MASK = %X (%d)\n", sname[site], mir, nparms[ip], nparms[ip]);
		  continue;
		}
	      
	      // Read in results for this site, mirror, and parameter mask

	      sprintf(command,"grep \"%d %d %d \" ./mirror_geometry.nparm.dat | grep -v \" %d %d %d \"", site, mir, nparms[ip], site, mir, nparms[ip]);
	      if((pp = popen(command,"r"))==NULL)exit(-1);
	      nc = 0;
	      while(fgets(buffer,2047,pp) != NULL)
		{
		  if(sscanf(buffer,"%d %d %d %d %lf %lf %lf %lf %lf %lf %lf",
			    &site0[nc], &mir0[nc], &nparms0[nc], &iboot[nc],
			    &phi[nc], &theta[nc], &psi[nc], &mcd[nc], 
			    &roc[nc], &nstars[nc], &chi2[nc]) != 11)continue;
		  
		  if(site0[nc] != site)continue;
		  if(mir0[nc] != mir)continue;
		  if(nparms0[nc] != nparms[ip])continue;
		  nc++;
		}
	      pclose(pp);
		  
	      if(nc<900)exit(-1); // Must have at least 900 results

	      // Open output file for this parameter mask setting

	      sprintf(fname,"mirror_geometry.%dparm.stats.csv", nparms[ip]);

	      if((fp=fopen(fname,"a"))==NULL){fprintf(stderr,"FATAL ERROR - unable to open \"%s\"\n", fname); exit(-1);}

	      fprintf(fp,"%d %d ", site, mir);

	      // Qsort data, calculate median and 95% CL, and print

	      qsort(phi, nc, sizeof(double),  (__compar_fn_t)compare_doubles);
	      fprintf(fp,"%.4lf %.4lf ", phi[nc/2], 2.0e0*(phi[(3*nc)/4]-phi[nc/4])/1.349e0);

	      qsort(theta, nc, sizeof(double),  (__compar_fn_t)compare_doubles);
	      fprintf(fp,"%.4lf %.4lf ", theta[nc/2], 2.0e0*(theta[(3*nc)/4]-theta[nc/4])/1.349e0);

	      qsort(psi, nc, sizeof(double),  (__compar_fn_t)compare_doubles);
	      fprintf(fp,"%.4lf %.4lf ", psi[nc/2], 2.0e0*(psi[(3*nc)/4]-psi[nc/4])/1.349e0);

	      qsort(mcd, nc, sizeof(double),  (__compar_fn_t)compare_doubles);
	      fprintf(fp,"%.4lf %.4lf ", mcd[nc/2], 2.0e0*(mcd[(3*nc)/4]-mcd[nc/4])/1.349e0);

	      qsort(roc, nc, sizeof(double),  (__compar_fn_t)compare_doubles);
	      fprintf(fp,"%.4lf %.4lf ", roc[nc/2], 2.0e0*(roc[(3*nc)/4]-roc[nc/4])/1.349e0);

	      fprintf(fp,"\n");

	      fclose(fp);
	    }
	}
    }
  
  return 0;
}


static int compare_doubles (const double * x, const double * y)
{
  if ((*x) > (*y))
    return 1;
  else if ((*x) < (*y))
    return -1;

  return 0;
}
