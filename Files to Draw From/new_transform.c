/*
   Routine to determine transformation parameters to adjust measured star positions. -SBT
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "julian.h"

#define __USE_GNU
#include <string.h>

#define d0(A,B) sqrt(pow((A).x-(B).x,2.0e0)+pow((A).y-(B).y,2.0e0))
#define xtrans(A) (alpha*D0*((cos(psi_t)*cos(theta_t)*cos(phi_t)-sin(psi_t)*sin(phi_t))*(A).x-(cos(psi_t)*cos(theta_t)*sin(phi_t)+sin(psi_t)*cos(phi_t))*(A).y-cos(psi_t)*sin(theta_t)*D0)/(sin(theta_t)*cos(phi_t)*(A).x+sin(theta_t)*sin(phi_t)*(A).y+cos(theta_t)*D0))
#define ytrans(A) (alpha*D0*(-(sin(psi_t)*cos(theta_t)*cos(phi_t)+cos(psi_t)*sin(phi_t))*(A).x+(cos(psi_t)*cos(phi_t)-sin(psi_t)*cos(theta_t)*sin(phi_t))*(A).y-sin(psi_t)*sin(theta_t)*D0)/(sin(theta_t)*cos(phi_t)*(A).x+sin(theta_t)*sin(phi_t)*(A).y+cos(theta_t)*D0))
#define xoff(A,B) ((B).x - xtrans(A))
#define yoff(A,B) ((B).y - ytrans(A))

#define RMAX 1.5e0

typedef struct VECTOR
{
  double x, y;
  double x0, y0;
} VECTOR;

typedef struct STAR
{
  char name[12];
  double theta, phi;
  double x, y;
  double x0, y0;
  double vmag;
  int istar;
} STAR;

int main (int argc, char *argv[]);

int main (int argc, char *argv[])
{
  FILE *fp, *pp, *lp;
  VECTOR tmp;
  VECTOR ul, ur, ll, lr, cc;
  VECTOR ul0, ur0, ll0, lr0, cc0;
  VECTOR ul1, ur1, ll1, lr1, cc1;
  static VECTOR s[2000], corner[5];	// Up to 2000 star
  double scale;
  double theta, thetamin, Smin, S, x0, y0, x0min, y0min;
  int site, mir, i, j, N, NC, NCC, NSTARS;
  char buffer[1025], fname[256], *token;
  char command[1025];
  int mtype, MX, MY, M;
  int year, month, day, hour, minute, second, duration;
  double seconds;
  double pressure = 840.0e0;
  double temperature = 20.0e0;
  static STAR star[5000], s1[5000];
  int NM = 0;
  int MSTARS;
  double SRADIUS;
  double vmin, vmax;
  double sdist, smin;
  char *fileroot, *filepath, *csvfile, *fullroot;
  double WIDTH, HEIGHT;
  char *sitename[3] = { "Middle Drum", "Black Rock", "Long Ridge" };
  char tmpname[20];
  double dMAX, dERR;
  int DUPLICATE;
  double dist, dmin;
  int jstar, ilast;
  double XOFFSET, YOFFSET;
  double jday;

  double alpha, D0, psi_t, phi_t, theta_t;

  if (argc < 1)
    {
      fprintf (stderr, "ERROR: Usage: %s </path/to/files/>\7\7\n", argv[0]);
      exit (-1);
    }

  fprintf (stderr, "path: %s\n", argv[1]);

  sprintf (command, "find %s -type f -name \"img_????.???.csv\"", argv[1]);
  fprintf (stderr, "command: %s\n", command);
  lp = popen (command, "r");

  while (fgets (buffer, 1024, lp) != NULL)
    {
      buffer[strlen (buffer) - 1] = '\0';
      csvfile = strdup (buffer);
      filepath = strndup (buffer, strlen (buffer) - 17);

      fprintf (stderr, "path: %s\n", filepath);
      fileroot = strndup (&buffer[strlen (buffer) - 16], 8);
      fullroot = strndup (&buffer[strlen (buffer) - 16], 12);

      sprintf (fname, "results/%s.stars.txt", fullroot);
      if(access( fname, F_OK ) != -1 )continue; // Results file exists

      // Extract image metadata

      // Creation date

      sprintf (fname, "exiftool -CreateDate %s/%s.jpg", filepath, fileroot);
      pp = popen (fname, "r");

      if (fgets (buffer, 1023, pp) == NULL)
	{
	  fprintf (stderr,
		   "ERROR: Failed to extract Create Date from %s/%s.jpg\7\7\n",
		   filepath, fileroot);
	  pclose (pp);
	  continue;
	}
      pclose (pp);

      if (sscanf(buffer, "Create Date : %d:%d:%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
	{
	  fprintf (stderr, "Failed to read Create Date from %s/%s.jpg\7\7\n", filepath, fileroot);
	  exit (-1);
	}


      // Exposure time

      sprintf (fname, "exiftool -ExposureTime %s/%s.jpg", filepath, fileroot);
      pp = popen (fname, "r");

      if (fgets (buffer, 1023, pp) == NULL)
	{
	  fprintf (stderr, "ERROR: Failed to extract Exposure Time from %s/%s.jpg\7\7\n",
		   filepath, fileroot);
	  pclose (pp);
	  continue;
	}
      pclose (pp);

      if (sscanf (buffer, "Exposure Time : %d", &duration) != 1)
	{
	  fprintf (stderr, "ERROR: Failed to read Exposure Time from %s/%s.jpg\7\7\n",
		   filepath, fileroot);
	  continue;
	}


      // TA Site Number

      sprintf (fname, "exiftool -TA_SiteNumber %s/%s.jpg", filepath, fileroot);
      pp = popen (fname, "r");

      if (fgets (buffer, 1023, pp) == NULL)
	{
	  fprintf (stderr, "ERROR: Failed to extract TA Site Number from %s/%s.jpg\7\7\n",
		   filepath, fileroot);
	  pclose (pp);
	  continue;
	}
      pclose (pp);

      if (sscanf (buffer, "TA Site Number : %d", &site) != 1)
	{
	  fprintf (stderr, "ERROR: Failed to read TA Site Number from %s/%s.jpg\7\7\n",
		   filepath, fileroot);
	  continue;
	}

      // TA Site Name

      sprintf (fname, "exiftool -TA_SiteName %s/%s.jpg", filepath, fileroot);
      pp = popen (fname, "r");

      if (fgets (buffer, 1023, pp) == NULL)
	{
	  fprintf (stderr, "ERROR: Failed to extract TA Site Name from %s/%s.jpg\7\7\n",
		   filepath, fileroot);
	  pclose (pp);
	  continue;
	}
      pclose (pp);

      if (strstr(buffer,"TA Site Name")==NULL)
	{
	  fprintf (stderr, "ERROR: Failed to locate TA Site Name in %s/%s.jpg\7\7\n",
		   filepath, fileroot);
	  continue;
	}
      

      buffer[strlen(buffer)-1] = '\0';
      token = strstr(buffer,": ");
      strcpy(tmpname, &token[2]);

      // TA Mirror Number

      sprintf (fname, "exiftool -TA_MirrorNumber %s/%s.jpg", filepath, fileroot);
      pp = popen (fname, "r");

      if (fgets (buffer, 1023, pp) == NULL)
	{
	  fprintf (stderr, "ERROR: Failed to extract TA Mirror Number from %s/%s.jpg\7\7\n",
		   filepath, fileroot);
	  pclose (pp);
	  continue;
	}
      pclose (pp);

      if (sscanf (buffer, "TA Mirror Number : %d", &mir) != 1)
	{
	  fprintf (stderr, "ERROR: Failed to read TA Mirror Number from %s/%s.jpg\7\7\n",
		   filepath, fileroot);
	  continue;
	}

      printf ("Extracted image metadata from %s/%s.jpg\n\n", filepath, fileroot);

      fprintf (stderr, "root: %s\n", fileroot);

      if((site<0)||(site>2))
	{
	  fprintf (stderr, "ERROR: Invalid site #%d found in image\7\7\n", site);
	  continue;
	}

      if(strcmp(sitename[site],tmpname)!=0)
	{
	  fprintf (stderr, "ERROR: Site name from image \"%s\" does not match site #%d = \"%s\"\7\7\n",  tmpname, site, sitename[site]);
	  continue;
	}

      if(site == 0)
	{
	  if((mir < 1) || (mir > 24))
	    {
	      fprintf (stderr, "ERROR: Invalid mirror number %s m%.2d\7\7\n",  sitename[site], mir);
	      continue;
	    }
	}
      else
	{
	  if ((mir < 0) || (mir > 11))
	    {
	      fprintf (stderr, "ERROR: Invalid mirror number %s m%.2d\7\7\n", sitename[site], mir);
	      continue;
	    }
	}

      // At this point I should have a validated site number and mirror number extracted from the image metadata
      // Now I can look through my corner_geometry.dat file to see if I can find matching corner data.

      WIDTH = HEIGHT = 0.0e0;

      sprintf (fname, "grep \"%d%.2d%.2d\t%d\t%d\" ./corner_geometry.dat", year, month, day, site, mir);
      pp = popen (fname, "r");

      if (fgets (buffer, 1023, pp) == NULL)
	{
	  fprintf (stderr, "ERROR: Failed to find corner data for %d%.2d%.2d site: %d mir: %d\7\7\n", year, month, day, site, mir);
	  continue;
	}

      if (sscanf (buffer, "%*d %*d %*d %lf %lf %lf %lf", &WIDTH, &HEIGHT, &XOFFSET, &YOFFSET) != 4)
	{
	  fprintf (stderr, "ERROR: Failed to read WIDTH, HEIGHT, XOFFSET, and YOFFSET from corner_geometry.dat\7\7\n");

	  continue;
	}

      pclose (pp);

      if(site>0)  // Convert to inches for BR & LR sites
	{
	  WIDTH /= 25.4e0;
	  HEIGHT /= 25.4e0;
	  XOFFSET /= 25.4e0;
	  YOFFSET /= 25.4e0;
	}

      ul0.x = -WIDTH / 2.0e0;
      ul0.y = HEIGHT / 2.0e0;

      ur0.x = WIDTH / 2.0e0;
      ur0.y = HEIGHT / 2.0e0;

      ll0.x = -WIDTH / 2.0e0;
      ll0.y = -HEIGHT / 2.0e0;

      lr0.x = WIDTH / 2.0e0;
      lr0.y = -HEIGHT / 2.0e0 ;

      cc0.x = 0.0e0;
      cc0.y = 0.0e0;

      sprintf (fname, "%s", csvfile);

      if ((fp = fopen (fname, "r")) == NULL)
	{
	  fprintf (stderr, "ERROR: Unable to open measurement file \"%s\"\7\7\n", fname);
	  continue;
	}
      else
	{
	  printf ("Reading measurement file %s\n", fname);
	}

      MX = MY = 0;
      NCC = NC = N = 0;

      while (fgets (buffer, 1023, fp) != NULL)
	{
	  buffer[strlen(buffer)-1] = '\0';

	  mtype = 0;
	  if (strcasestr (buffer, "center") != NULL)mtype = 1;
	  else if (strcasestr (buffer, "corner") != NULL)mtype = 2;
	  else if (strcasestr (buffer, "star") != NULL)mtype = 3;

	  M = 0;
	  tmp.x = tmp.y = (1000.0e0);

	  if ((token = strtok (buffer, ", ;\t")) == NULL)continue;
	  if(strcasestr(token,"area")!=NULL)M=2;

	  while ((token = strtok (NULL, ", ;\t")) != NULL)
	    {
	      M++;
	  
	      if (MX == 0)
		{
		  if (strstr (token, "XM") != NULL)MX = M;
		}
	      if (MY == 0)
		{
		  if (strstr (token, "YM") != NULL)MY = M;
		}

	      if ((MX > 0) && (MY > 0))
		{
		  if (site == 0)
		    {
		      if (M == (MX - 1))tmp.x = atof (token);
		      if (M == (MY - 1))tmp.y = atof (token);
		    }
		  else
		    {
		      if (M == (MX - 1))tmp.x = atof (token) / 25.4e0;
		      if (M == (MY - 1))tmp.y = atof (token) / 25.4e0;
		    }
		}
	    }

	  if (abs(tmp.x) > 2.0e0 * WIDTH)continue;
	  if (abs(tmp.y) > 2.0e0 * HEIGHT)continue;

	  if (mtype == 1)
	    {
	      cc.x = tmp.x;
	      cc.y = tmp.y;
	      NCC = 1;
	    }			// Center
	  else if (mtype == 3)
	    {
	      s[N].x0 = s[N].x = tmp.x;
	      s[N].y0 = s[N].y = tmp.y;

	      if(site>0)
		{
		  s[N].x0 *= 25.4e0;
		  s[N].y0 *= 25.4e0;
		}
	      N++;
	    }			// Star
	  else if (mtype == 2)
	    {
	      corner[NC].x = tmp.x;
	      corner[NC].y = tmp.y;
	      NC++;
	    }			// Corner
	}

      fclose (fp);

      if (NCC != 1)
	{
	  fprintf(stderr,"mv %s %s.badcorners\n", csvfile, csvfile);
	  sprintf(command,"mv %s %s.badcorners\n", csvfile, csvfile);
	  system(command);

	  sprintf(fname,"%s.processing_report", csvfile, csvfile);

	  if ((fp = fopen (fname, "w")) == NULL)
	    {
	      fprintf (stderr, "ERROR: Unable to write processing report file %s\7\7\n", fname);
	      exit(-1);
	    }

	  fprintf(fp,"Processing report:\n\n");
	  fprintf (fp, "Failed to find center.\7\7\n");

	  fclose(fp);
	  continue;
	}

      if (NC != 4)
	{
	  fprintf(stderr,"mv %s %s.badcorners\n", csvfile, csvfile);
	  sprintf(command,"mv %s %s.badcorners\n", csvfile, csvfile);
	  system(command);

	  sprintf(fname,"%s.processing_report", csvfile, csvfile);

	  if ((fp = fopen (fname, "w")) == NULL)
	    {
	      fprintf (stderr, "ERROR: Unable to write processing report file %s\7\7\n", fname);
	      exit(-1);
	    }

	  fprintf(fp,"Processing report:\n\n");
	  fprintf(fp, "Failed to find four corners.\7\7\n");

	  fclose(fp);

	  continue;
	}

      // Set the origin to the center

      for (i = 0; i < NC; i++)
	{
	  printf ("%d: (%.2lf, %.2lf) ", i + 1, corner[i].x, corner[i].y);
	  printf ("- (%.2lf, %.2lf) ", cc.x, cc.y);
	  corner[i].x -= cc.x;
	  corner[i].y -= cc.y;
	  printf ("= (%.2lf, %.2lf)\n", corner[i].x, corner[i].y);
	}
      for (i = 0; i < N; i++)
	{
	  s[i].x -= cc.x;
	  s[i].y -= cc.y;
	}

      // Set center to zero

      cc.x = 0.0e0;
      cc.y = 0.0e0;

      // Identify the corners

      NM = 0;
      for (i = 0; i < NC; i++)
	{
	  if (d0 (corner[i], ul0) < WIDTH / 10.0e0)
	    {
	      ul.x = corner[i].x;
	      ul.y = corner[i].y;
	      NM++;
	    }
	  if (d0 (corner[i], ur0) < WIDTH / 10.0e0)
	    {
	      ur.x = corner[i].x;
	      ur.y = corner[i].y;
	      NM++;
	    }
	  if (d0 (corner[i], ll0) < WIDTH / 10.0e0)
	    {
	      ll.x = corner[i].x;
	      ll.y = corner[i].y;
	      NM++;
	    }
	  if (d0 (corner[i], lr0) < WIDTH / 10.0e0)
	    {
	      lr.x = corner[i].x;
	      lr.y = corner[i].y;
	      NM++;
	    }
	}

      if (NM == 4)
	{
	  printf ("ul: %.3lf %.3lf\n", ul.x, ul.y);
	  printf ("ur: %.3lf %.3lf\n", ur.x, ur.y);
	  printf ("ll: %.3lf %.3lf\n", ll.x, ll.y);
	  printf ("lr: %.3lf %.3lf\n", lr.x, lr.y);
	  printf ("cc: %.3lf %.3lf\n", cc.x, cc.y);
	}
      else
	{
	  fprintf (stderr, "ERROR: Failed to identify the four corners\7\7\n");

	  fprintf(stderr,"mv %s %s.badcorners\n", csvfile, csvfile);
	  sprintf(command,"mv %s %s.badcorners\n", csvfile, csvfile);
	  system(command);

	  sprintf(fname,"%s.processing_report", csvfile, csvfile);

	  if ((fp = fopen (fname, "w")) == NULL)
	    {
	      fprintf (stderr, "ERROR: Unable to write processing report file %s\7\7\n", fname);
	      exit(-1);
	    }

	  fprintf(fp,"Processing report:\n");
	  fprintf(fp, "Failed to identify four corners.\n\n");
	  fprintf (fp,"ul0: %.3lf %.3lf\n", ul0.x, ul0.y);
	  fprintf (fp,"ur0: %.3lf %.3lf\n", ur0.x, ur0.y);
	  fprintf (fp,"ll0: %.3lf %.3lf\n", ll0.x, ll0.y);
	  fprintf (fp,"lr0: %.3lf %.3lf\n", lr0.x, lr0.y);

	  for (i = 0; i < NC; i++)
	    {
	      fprintf (fp,"corner[%d]: %.3lf %.3lf\n", i+1, corner[i].x, corner[i].y);
	    }

	  fclose(fp);

	  continue;
	}

      for (i = 0; i < N; i++)
	printf ("star[%d]: %.3lf %.3lf\n", i + 1, s[i].x, s[i].y);
      printf ("\n");

      fflush (stdout);

      // Use rotation and offset invariant distances to set the scale

      scale  = d0 (ul0, lr0) / d0 (ul, lr);
      scale += d0 (ur0, ll0) / d0 (ur, ll);
      scale += d0 (ur0, ul0) / d0 (ur, ul);
      scale += d0 (lr0, ll0) / d0 (lr, ll);
      scale += d0 (ul0, ll0) / d0 (ul, ll);
      scale += d0 (ur0, lr0) / d0 (ur, lr);
      scale += d0 (ul0, cc0) / d0 (ul, cc);
      scale += d0 (ur0, cc0) / d0 (ur, cc);
      scale += d0 (ll0, cc0) / d0 (ll, cc);
      scale += d0 (lr0, cc0) / d0 (lr, cc);
      scale /= 10.0e0;

      if (fabs (100.0e0 * (scale - 1.0e0)) > 1.0e0)	// 1 percent maximum
	{
	  fprintf (stderr, "Scale \"%.3lf\" is out of range!\7\7\n", scale);
	  fprintf(stderr,"mv %s %s.badcorners\n", csvfile, csvfile);
	  sprintf(command,"mv %s %s.badcorners\n", csvfile, csvfile);
	  system(command);

	  sprintf(fname,"%s.processing_report", csvfile, csvfile);

	  if ((fp = fopen (fname, "w")) == NULL)
	    {
	      fprintf (stderr, "ERROR: Unable to write processing report file %s\7\7\n", fname);
	      exit(-1);
	    }

	  fprintf(fp,"Processing report:\n\n");
	  fprintf(fp,"Scale \"%.3lf\" is out of range! Should be close to 1.000.\n\n", scale);
	  fprintf(fp,"Are you sure you set the correct distance for measuring?\n");
	  fclose(fp);

	  continue;
	}

      // Try out new transform

      D0      = (476.0e0*(1.0e0-0.485e0)/2.54e0);
      alpha   = scale;
      psi_t   = 0.0e0;
      phi_t   = 0.0e0;
      theta_t = 0.0e0;

      // How to minimize this?

      ul1.x = xtrans (ul);
      ul1.y = ytrans (ul);
      ur1.x = xtrans (ur);
      ur1.y = ytrans (ur);
      ll1.x = xtrans (ll);
      ll1.y = ytrans (ll);
      lr1.x = xtrans (lr);
      lr1.y = ytrans (lr);
      cc1.x = xtrans (cc);
      cc1.y = ytrans (cc);

      S = 0.0e0;
      S += fabs(ul0.x - ul1.x);
      S += fabs(ul0.y - ul1.y);
      S += fabs(ur0.x - ur1.x);
      S += fabs(ur0.y - ur1.y);
      S += fabs(ll0.x - ll1.x);
      S += fabs(ll0.y - ll1.y);
      S += fabs(lr0.x - lr1.x);
      S += fabs(lr0.y - lr1.y);
      S += fabs(cc0.x - cc1.x);
      S += fabs(cc0.y - cc1.y);
      S += fabs(d0(ul1, lr1) - d0(ul0, lr0));
      S += fabs(d0(ur1, ll1) - d0(ur0, ll0));
      S += fabs(d0(ul1, ll1) - d0(ul0, ll0));
      S += fabs(d0(ur1, lr1) - d0(ur0, lr0));
      S += fabs(d0(ul1, ur1) - d0(ul0, ur0));
      S += fabs(d0(ll1, lr1) - d0(ll0, lr0));
      S += fabs(d0(ul1, cc1) - d0(ul0, cc0));
      S += fabs(d0(ur1, cc1) - d0(ur0, cc0));
      S += fabs(d0(ll1, cc1) - d0(ll0, cc0));
      S += fabs(d0(lr1, cc1) - d0(lr0, cc0));
      
      printf ("\n\n");
      printf ("scale = %.5lf\n", scale);
      printf ("D0    = %.5lf\n", D0);
      printf ("theta = %.5lf\n", theta_t * 180.0e0 / M_PI);
      printf ("phi = %.5lf\n", phi_t * 180.0e0 / M_PI);
      printf ("psi = %.5lf\n", psi_t * 180.0e0 / M_PI);
      printf ("Smin = %.6le\n\n", S);

      ul1.x = xtrans (ul);
      ul1.y = ytrans (ul);
      ur1.x = xtrans (ur);
      ur1.y = ytrans (ur);
      ll1.x = xtrans (ll);
      ll1.y = ytrans (ll);
      lr1.x = xtrans (lr);
      lr1.y = ytrans (lr);
      cc1.x = xtrans (cc);
      cc1.y = ytrans (cc);

      printf ("dUL = %.3lf %.3lf\n", ul0.x - ul1.x, ul0.y - ul1.y);
      printf ("dUR = %.3lf %.3lf\n", ur0.x - ur1.x, ur0.y - ur1.y);
      printf ("dLL = %.3lf %.3lf\n", ll0.x - ll1.x, ll0.y - ll1.y);
      printf ("dLR = %.3lf %.3lf\n", lr0.x - lr1.x, lr0.y - lr1.y);
      printf ("dCC = %.3lf %.3lf\n", cc0.x - cc1.x, cc0.y - cc1.y);
      printf ("\n");
      printf ("dUL-LR = %.3lf\n", d0 (ul1, lr1) - d0 (ul0, lr0));
      printf ("dUR-LL = %.3lf\n", d0 (ur1, ll1) - d0 (ur0, ll0));
      printf ("dUL-LL = %.3lf\n", d0 (ul1, ll1) - d0 (ul0, ll0));
      printf ("dUR-LR = %.3lf\n", d0 (ur1, lr1) - d0 (ur0, lr0));
      printf ("dUL-UR = %.3lf\n", d0 (ul1, ur1) - d0 (ul0, ur0));
      printf ("dLL-LR = %.3lf\n", d0 (ll1, lr1) - d0 (ll0, lr0));
      printf ("dUL-CC = %.3lf\n", d0 (ul1, cc1) - d0 (ul0, cc0));
      printf ("dUR-CC = %.3lf\n", d0 (ur1, cc1) - d0 (ur0, cc0));
      printf ("dLL-CC = %.3lf\n", d0 (ll1, cc1) - d0 (ll0, cc0));
      printf ("dLR-CC = %.3lf\n", d0 (lr1, cc1) - d0 (lr0, cc0));
      printf ("\n");

      dMAX = sqrt(pow(ul0.x - ul1.x, 2.0e0)+pow(ul0.y - ul1.y,2.0e0));
      dERR = sqrt(pow(ur0.x - ur1.x, 2.0e0)+pow(ur0.y - ur1.y,2.0e0)); if(dERR>dMAX)dMAX=dERR;
      dERR = sqrt(pow(ll0.x - ll1.x, 2.0e0)+pow(ll0.y - ll1.y,2.0e0)); if(dERR>dMAX)dMAX=dERR;
      dERR = sqrt(pow(lr0.x - lr1.x, 2.0e0)+pow(lr0.y - lr1.y,2.0e0)); if(dERR>dMAX)dMAX=dERR;
      dERR = sqrt(pow(cc0.x - cc1.x, 2.0e0)+pow(cc0.y - cc1.y,2.0e0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (ul1, lr1) - d0 (ul0, lr0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (ur1, ll1) - d0 (ur0, ll0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (ul1, ll1) - d0 (ul0, ll0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (ur1, lr1) - d0 (ur0, lr0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (ul1, ur1) - d0 (ul0, ur0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (ll1, lr1) - d0 (ll0, lr0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (ul1, cc1) - d0 (ul0, cc0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (ur1, cc1) - d0 (ur0, cc0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (ll1, cc1) - d0 (ll0, cc0)); if(dERR>dMAX)dMAX=dERR;
      dERR = fabs(d0 (lr1, cc1) - d0 (lr0, cc0)); if(dERR>dMAX)dMAX=dERR;

      // Maximum allowed corner error is 0.125"

      if(dMAX > 0.260e0) // Increased to 0.260"
	{
	  fprintf(stderr,"mv %s %s.badcorners\n", csvfile, csvfile);
	  sprintf(command,"mv %s %s.badcorners\n", csvfile, csvfile);
	  system(command);

	  sprintf(fname,"%s.processing_report", csvfile, csvfile);

	  if ((fp = fopen (fname, "w")) == NULL)
	    {
	      fprintf (stderr, "ERROR: Unable to write processing report file %s\7\7\n", fname);
	      exit(-1);
	    }

	  fprintf(fp,"Processing report:\n\n");
	  fprintf(fp,"Maximum corner error exceeded err = %.3lf\n\n", dMAX);

	  fprintf(fp,"dUL = %.3lf %.3lf\n", ul0.x - ul1.x, ul0.y - ul1.y);
	  fprintf(fp,"dUR = %.3lf %.3lf\n", ur0.x - ur1.x, ur0.y - ur1.y);
	  fprintf(fp,"dLL = %.3lf %.3lf\n", ll0.x - ll1.x, ll0.y - ll1.y);
	  fprintf(fp,"dLR = %.3lf %.3lf\n", lr0.x - lr1.x, lr0.y - lr1.y);
	  fprintf(fp,"dCC = %.3lf %.3lf\n", cc0.x - cc1.x, cc0.y - cc1.y);
	  fprintf(fp,"\n");
	  fprintf(fp,"dUL-LR = %.3lf\n", d0 (ul1, lr1) - d0 (ul0, lr0));
	  fprintf(fp,"dUR-LL = %.3lf\n", d0 (ur1, ll1) - d0 (ur0, ll0));
	  fprintf(fp,"dUL-LL = %.3lf\n", d0 (ul1, ll1) - d0 (ul0, ll0));
	  fprintf(fp,"dUR-LR = %.3lf\n", d0 (ur1, lr1) - d0 (ur0, lr0));
	  fprintf(fp,"dUL-UR = %.3lf\n", d0 (ul1, ur1) - d0 (ul0, ur0));
	  fprintf(fp,"dLL-LR = %.3lf\n", d0 (ll1, lr1) - d0 (ll0, lr0));
	  fprintf(fp,"dUL-CC = %.3lf\n", d0 (ul1, cc1) - d0 (ul0, cc0));
	  fprintf(fp,"dUR-CC = %.3lf\n", d0 (ur1, cc1) - d0 (ur0, cc0));
	  fprintf(fp,"dLL-CC = %.3lf\n", d0 (ll1, cc1) - d0 (ll0, cc0));
	  fprintf(fp,"dLR-CC = %.3lf\n", d0 (lr1, cc1) - d0 (lr0, cc0));
	  fprintf(fp,"\n");
	  
	  fclose(fp);

	  continue;
	}

      for (i = 0; i < N; i++)
	{
	  s1[i].x = xtrans(s[i]) + XOFFSET;  // Add offset to true cluster center
	  s1[i].y = ytrans(s[i]) + YOFFSET;  //
	  s1[i].x0 = s[i].x0;
	  s1[i].y0 = s[i].y0;
	  s1[i].vmag = 30.0e0;

	  printf("star[%d] = %d %.4d/%.2d/%.2d %.2d:%.2d:%.2d %d %.4lf %.4lf\n",
		 i + 1, mir, year, month, day, hour, minute, second, duration, s1[i].x, s1[i].y);
	}

      printf ("\n\n");
      fflush (stdout);

      // Now lets see if we can identify any of these stars
      // According to Tom the time stamp is the beginning of the exposure

      jday = julian( year, month, day, hour, minute, (double)second);
      jday += (double)duration/2.0e0/86400.0e0;

      jdate( jday, &year, &month, &day, &hour, &minute, &seconds);

      sprintf (fname, "./ta_stars %d %d %d %d %d %d %d %.6lf %.2lf %.2lf | sort -k 6 -g",
	       site, mir, year, month, day, hour, minute, floor(seconds*1.0e5)/1.0e5,
	       temperature, pressure);

      printf ("Looking for stars: %s\n", fname);

      pp = popen (fname, "r");

      NSTARS = 0;
      while (fgets (buffer, 1023, pp) != NULL)
	{
	  if (sscanf (buffer, "%s %lf %lf %lf %lf %lf",
		      star[NSTARS].name,
		      &(star[NSTARS].theta), &(star[NSTARS].phi),
		      &(star[NSTARS].x), &(star[NSTARS].y),
		      &(star[NSTARS].vmag)) == 6)
	    NSTARS++;
	}

      pclose (pp);

      // Clear list of matched stars

      for (i = 0; i < NSTARS; i++)star[i].istar = (-1);

      // Look for stars matching measured locations

      for (j = 0; j < N; j++)
	{
	  // Find the closest star to this location
	  smin = 1.0e23;
	  ilast = (-1);
	  for (i = 0; i < NSTARS; i++) 
	    {
	      if(star[i].istar < 0) // Not all ready matched
		{
		  sdist = d0(star[i], s1[j]);
		  if ((sdist<RMAX)&&(sdist < smin))
		    {
		      if(s1[j].vmag+1.0e0 > star[i].vmag)
			{
			  s1[j].vmag = star[i].vmag;
			  star[i].istar = j;
			  if(ilast>=0)star[ilast].istar = (-1);
			  ilast = i;
			  smin = sdist;
			}
		    }
		}
	    }
	}

      // Look for duplicate matches

      for (i = 0; i < NSTARS; i++)
	{
	  if(star[i].istar<0)continue; // No match
	  for (j = 0; j < NSTARS; j++)
	    {
	      if(star[j].istar<0)continue; // No match
	      {
		if(i==j)continue; // Same star
		if(star[i].istar == star[j].istar) // Duplicate star match - danger
		  {
		    if(d0(star[i], s1[star[i].istar]) < d0(star[j], s1[star[j].istar]))
		      {
			star[j].istar = (-1);
		      }
		    else
		      {
			star[i].istar = (-1);
		      }
		  }
	      }
	    }
	}

      MSTARS = 0; DUPLICATE = (-1);
      for (i = 0; i < NSTARS; i++)
	{
	  if(star[i].istar<0)continue; // No match
	  for (j = 0; j < NSTARS; j++)
	    {
	      if(star[j].istar<0)continue; // No match
	      {
		if(i==j)continue; // Same star
		if(star[i].istar == star[j].istar) // Duplicate star match - danger
		  {
		    DUPLICATE = 1;
		  }
	      }
	    }
	  if(DUPLICATE == 1)
	    {
	      MSTARS = 0;
	      break;
	    }
	  else
	    {
	      MSTARS++;
	    }
	}
      
      if(MSTARS<N)
	{
	  fprintf(stderr,"mv %s %s.matched_%d_out_of_%d_stars\n", csvfile, csvfile, MSTARS, N);
	  sprintf(command,"mv %s %s.matched_%d_out_of_%d_stars\n", csvfile, csvfile, MSTARS, N);
	  system(command);
	  sprintf(fname,"%s.processing_report", csvfile, csvfile);

	  if ((fp = fopen (fname, "w")) == NULL)
	    {
	      fprintf (stderr, "ERROR: Unable to write processing report file %s\7\7\n", fname);
	      exit(-1);
	    }

	  fprintf(fp,"Processing report:\n\n");
	  fprintf(fp,"%s m%.2d %d/%.2d/%d %.2d:%.2d:%.2lf\n", sitename[site], mir, month, day, year, hour, minute, seconds);
	  fprintf(fp,"Photo: %s\n\n", csvfile);

	  fprintf(fp,"%d stars were found in the CSV file.\n\n", N);

	  for (j = 0; j < N; j++)
	    {
	      fprintf(fp,"%d. (%.3lf,%.3lf)\n", j+1, s1[j].x0, s1[j].y0);
	    }

	  fprintf(fp,"\n%d out of %d potential matches were identified.\n\n", MSTARS, NSTARS);

	  for (j = 0; j < N; j++)
	    {
	      for (i = 0; i < NSTARS; i++)
		{
		  if(star[i].istar == j)
		    {
		      fprintf(fp,"%d. (%.3lf,%.3lf) ", j+1, s1[star[i].istar].x0, s1[star[i].istar].y0);
		      fprintf(fp,"=> (%.3lf,%.3lf) ", s1[star[i].istar].x, s1[star[i].istar].y);
		      fprintf(fp,"-> %s mag: %.2lf (%.3lf,%.3lf) ", star[i].name, star[i].vmag, star[i].x, star[i].y);
		      fprintf(fp,"delta: (%.3lf"",%.3lf"")\n", s1[star[i].istar].x - star[i].x, s1[star[i].istar].y - star[i].y);
		      break;
		    }
		}
	      if(i==NSTARS) // No matching star found
		{
		  dmin = 1.0e23;
		  jstar = (-1);
		  for (i = 0; i < NSTARS; i++)
		    {
		      if(star[i].istar < 0) // No match
			{
			  if(star[i].istar<(-1))continue; // All ready matched another missing star
			  dist = d0(star[i], s1[j]);
			  if(dist<dmin)
			    {
			      dmin = dist;
			      jstar = i;
			    }
			}
		    }
		
		  fprintf(fp,"%d. (%.3lf,%.3lf) ", j+1, s1[j].x0, s1[j].y0);
		  fprintf(fp,"=> (%.3lf,%.3lf) not matched, closest star is ", s1[j].x, s1[j].y);
		  fprintf(fp,"%s mag: %.2lf (%.3lf,%.3lf) ", star[jstar].name, star[jstar].vmag, star[jstar].x, star[jstar].y);
		  fprintf(fp,"delta: (%.3lf"",%.3lf"")\n", s1[j].x - star[jstar].x, s1[j].y - star[jstar].y); 

		  star[jstar].istar = (-2);
		}
	    }

	  jstar = 1;
	  for (i = 0; i < NSTARS; i++)
	    {
	      if(star[i].istar == (-1))
		{
		  fprintf(fp,"%d. %s mag: %.2lf (%.3lf,%.3lf)\n", N+jstar++, star[i].name, star[i].vmag, star[i].x, star[i].y);
		}
	    }


	  if(DUPLICATE==1)
	    fprintf(fp,"\n\nDuplicate matches confused the analysis program. Do not include stars which are too close together.\n");

	  fclose(fp);
	  continue;
	}

      sprintf (fname, "results/%s.stars.txt", fullroot);
      if ((fp = fopen (fname, "w")) == NULL)
	{
	  fprintf (stderr, "ERROR: Unable to write results file %s\7\7\n", fname);
	  exit (-1);
	}

      for (j = 0; j < NSTARS; j++)
	{
	  if (star[j].istar >= 0)
	    {
	      if(d0(star[j], s1[star[j].istar])>RMAX)continue;
	      printf ("star[%d].istar = %d\n", j + 1, star[j].istar + 1);
	      fprintf (fp, "%d %d ", site + 1, mir);
	      fprintf (fp, "%.4d/%.2d/%.2d %.2d:%.2d:%.2lf %d ", year, month,
		       day, hour, minute, seconds, duration);
	      fprintf (fp, "%s %.4lf %.4lf %.4lf %.4lf ", star[j].name,
		       star[j].theta, star[j].phi, star[j].x, star[j].y);
	      fprintf (fp, "%.4lf %.4lf %.4lf\n", s1[star[j].istar].x,
		       s1[star[j].istar].y, d0 (star[j], s1[star[j].istar]));
	    }
	}

      fclose (fp);

      // Results file successfully written.

      fprintf(stderr,"mv %s %s.passed\n", csvfile, csvfile);
      sprintf(command,"mv %s %s.passed\n", csvfile, csvfile);
      system(command);

      // Let my people go

      free (fileroot);
      free (fullroot);
      free (filepath);
      free (csvfile);
    }

  pclose (lp);

  return 0;
}
