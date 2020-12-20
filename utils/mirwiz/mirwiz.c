/***************************************************************************

  mirwiz.c

  Parse mirror.ini file into subpart mirror table using l3p structs.

  NOTE: This will only mirror the main (first) file in an MPD.
  NOTE: This uppercases filenames.  Fixme.

***************************************************************************/

typedef unsigned long        COLORREF;

#include <stdio.h>
#include <stdlib.h>

#include "StdAfx.h"

#include "L3Def.h"
#include "math.h"

#define BUFSIZE (1024)
char buf[BUFSIZE];
char description[BUFSIZE];
static char          SubPartDatName[_MAX_PATH];

static char *PartCategories[] = 
{
  "[MIRRORSETTINGS]",
};

struct L3PartS      *CategoryPtr[15];
int                  CategorySize[15];

//struct L3PartS       Parts[MAX_PARTS];
struct L3PartS       Parts[8000];
int                  nParts = 0;              /* Number of parts in Parts[]      */

/***************************************************************/
// Extra stuff from ldglite to find LDRAWDIR (and thus MIRROR.INI)
/***************************************************************/
#include "platform.h"

int use_uppercase = 0;

char pathname[256];
char primitivepath[256];
char partspath[256];
char modelspath[256];
char datfilepath[256];

char userpath[256];
char bitmappath[256];

/***************************************************************/
int ldlite_parse_colour_meta(char *s)
{
  return 0; // Don't care about color for mirroring.
}

#if defined(UNIX) || defined(MAC)
// These should really move to platform.c and platform.h
/***************************************************************/
int GetProfileInt(char *appName, char *appVar, int varDefault)
{
  // This should look up appVar in some .INI or .RC file for appName.
  // Windows uses reg key HKCU\Software\Gyugyi Cybernetics\ldlite\Settings
  // Just return the default for now.
  return (varDefault);
}

/***************************************************************/
int GetPrivateProfileInt(char *appName, char *appVar, int varDefault, char *fileName)
{
  // Just return the default for now.
  return (varDefault);
}

/***************************************************************/
int GetPrivateProfileString(char *appName, char *appVar, char *varDefault, 
			    char *retString, int strSize, char *fileName)
{
  // Just return the default for now.
  strcpy (retString, varDefault);
  return (strlen(varDefault));
}
#else 
#  include <windows.h>
#endif

/***************************************************************/
void platform_setpath()
{
  char *env_str;

  use_uppercase = 0; // default
  env_str = platform_getenv("LDRAWDIRCASE");
  if (env_str != NULL)
  {
    if (stricmp(env_str, "UPPERCASE") == 0)
      use_uppercase = 1;
  }
  
  env_str = platform_getenv("LDRAWDIR");
  if (env_str != NULL)
  {
    strcpy(pathname, env_str);
  }
  else if (GetPrivateProfileString("LDraw","BaseDirectory","",
			  pathname,256,"ldraw.ini") == 0)
  {
#if defined MACOS_X
    sprintf(pathname, "/Library/ldraw");
#elif defined(UNIX)
    sprintf(pathname, "/usr/local/ldraw");
#elif defined(MAC)
    sprintf(pathname, "Aulus:Code:Lego.CAD:LDRAW");
#elif defined(WINDOWS)
    sprintf(pathname, "c:/ldraw/");
#else
#error unspecified platform in platform_getenv() definition
#endif
  }

  concat_path(pathname, use_uppercase ? "P" : "p", primitivepath);
  concat_path(pathname, use_uppercase ? "PARTS" : "parts", partspath);

  // Give the user a chance to override some paths for personal data.
  env_str = platform_getenv("LDRAWUSER");
  if (env_str != NULL)
  {
    strcpy(userpath, env_str);
  }
  else if (GetPrivateProfileString("LDraw","UserDirectory","",
			  userpath,256,"ldraw.ini") == 0)
  {
    strcpy(userpath, pathname);
  }

  concat_path(userpath, use_uppercase ? "MODELS" : "models", modelspath);
  concat_path(userpath, use_uppercase ? "BITMAP" : "bitmap", bitmappath);
}

//***************************************************************************
int mirrorinit(char *inifile)
{
  FILE * fp;
  int i, j, n;
  int category = -1;
  struct L3LineS       Data;
  struct L3PartS      *PartPtr;

  // NOTE:  Should probably look in LDRAWDIR for the ini file.
  char filename[256] = "mirror.ini"; // Default filename

  if (inifile)
    strcpy(filename, inifile);

  for (j = 0; j < 15; j++)
    CategorySize[j] = 0;

  fp = fopen(filename, "r");
  if (!fp)
    return 0;

  for(i = 0; ; i++)
  {
    register int len;
    char *p = &buf[0];
    
    if (!fp)
      break;
    if(! fgets(buf, BUFSIZE - 2, fp))
      break;
    if (buf[0] == ';')
    {
      // Skip comment
    }
    else if (buf[0] == '[') // else if (p = strchr(buf, '['))
    {
      for (j = 0; j < 15; j++)
      {
	if (!strncmp(buf, PartCategories[j], strlen(PartCategories[j])))
	{
	  category = j;
	  CategoryPtr[j] = &Parts[nParts];
	  break;
	}
      }
    }
    else if (category >= 0)
    {
      char *token;

      p = buf;
      token = strchr(p, '\"');
      if (!token)
	continue;
      token++;
      p = strchr(token, '\"');
      if (!p)
	continue;
      *p = 0;
      p++;
      strcpy(description, token);

      token = strchr(p, '\"');
      if (!token)
	continue;
      token++;
      p = strchr(token, '\"');
      if (!p)
	continue;
      *p = 0;
      p++;
      strcpy(SubPartDatName, token);
      p += strspn(p, " \t");

      memset(&Data, 0, sizeof(struct L3LineS));
      Data.v[3][3] = 1;
      n = sscanf(p, "%d %f %f %f %f %f %f %f %f %f %f %f %f",
                 &Data.LineType,
                 &Data.v[0][0], &Data.v[0][1], &Data.v[0][2],
                 &Data.v[1][0], &Data.v[1][1], &Data.v[1][2],
                 &Data.v[2][0], &Data.v[2][1], &Data.v[2][2],
		 &Data.v[0][3], &Data.v[1][3], &Data.v[2][3]);
#if 0
      printf(" %s <%s> [%s] %d %g %g %g %g %g %g %g %g %g (%g %g %g)\n",
	     PartCategories[j], description, SubPartDatName, n,
                 Data.v[0][0], Data.v[0][1], Data.v[0][2],
                 Data.v[1][0], Data.v[1][1], Data.v[1][2],
                 Data.v[2][0], Data.v[2][1], Data.v[2][2],
                 Data.v[0][3], Data.v[1][3], Data.v[2][3]);
#endif
      if (n != 13)
	continue;

      CategorySize[category]++;

      PartPtr = &Parts[nParts++];
      memset(PartPtr, 0, sizeof(struct L3PartS));  /* Clear all flags           */

      // NOTE: This uses description and SubPartDatName opposite from MLCAD.INI
      PartPtr->DatName = strdup(description);
      PartPtr->FirstLine = (struct L3LineS *) malloc(sizeof(struct L3LineS));
      memcpy(PartPtr->FirstLine, &Data, sizeof(struct L3LineS));
      PartPtr->FirstLine->Comment = strdup(SubPartDatName);
    }
  }
  
  if (fp)
    fclose(fp);
  
  return 1;
}

//---------------------------------------------------------------------------
void strupper(char *s) {
  char *p;
  for (p = s; *p; p++) {
    *p = toupper(*p);
  }
}

//---------------------------------------------------------------------------
#pragma argsused
int main(int argc, char* argv[])
{
  char *dat_name = argv[1];
  char *dst_name = argv[2];
  FILE *outfile;
  FILE *dat;
  char line[512];
  char *nonwhite;

  struct L3LineS  Data;
  struct L3LineS *LinePtr = &Data;
  struct L3PartS *PartPtr;

  int isMPD = 0;
  int i;

  printf("MirWiz version 0.1\n");

  if (argc == 2 && strcmp(argv[1],"-v") == 0) {
    return 1;
  }

  if (argc < 3) {
    printf("usage: mirwiz <input_file> <output_file>\n");
    return 1;
  }

  platform_setpath();
  concat_path(pathname, use_uppercase ? "MIRROR.INI" : "mirror.ini", datfilepath);

  if (!mirrorinit(datfilepath))
  {
    printf("Could not find config file %s\n", datfilepath);
    mirrorinit("mirror.ini"); // Try here?
  }

  dat = fopen(dat_name,"r");

  if (dat == NULL) {
    printf("%s: Failed to open file %s for reading\n",argv[0],dat_name);
    return -1;
  }

  outfile = fopen(dst_name,"w");

  if (outfile == NULL) {
    printf("%s: Failed to open file %s for writing\n",argv[0],dst_name);
    return -1;
  }

  // Consider using L3fgets() to avoid OS dependent line termination issues.
  while (fgets(line,sizeof(line), dat)) 
  {
    int color;
    int j,k;
    char datname[256];
    float m[4][4];
    float md[4][4] = { // Default = left-right mirror
      {-1.0,0.0,0.0,0.0},
      {0.0,1.0,0.0,0.0},
      {0.0,0.0,1.0,0.0},
      {0.0,0.0,0.0,1.0}
    };
    float *me = md; // Init exception mirror to left-right.
    float mm[4][4] = { // Front-back mirror
      {1.0,0.0,0.0,0.0},
      {0.0,1.0,0.0,0.0},
      {0.0,0.0,-1.0,0.0},
      {0.0,0.0,0.0,1.0}
    };
    //float *mp = md; // Init global mirror to left-right.
    float *mp = mm; // Init global mirror to front-back.

    // Strip away leading whitespace (spaces and tabs).
    nonwhite = line + strspn(line, " \t");

    memset(&Data, 0, sizeof(struct L3LineS));
    Data.v[3][3] = 1;
    if (sscanf(nonwhite,"%d %d %f %f %f %f %f %f %f %f %f %f %f %f %s",
	       &Data.LineType, &Data.Color,
	       &Data.v[0][3], &Data.v[1][3], &Data.v[2][3],
	       &Data.v[0][0], &Data.v[0][1], &Data.v[0][2],
	       &Data.v[1][0], &Data.v[1][1], &Data.v[1][2],
	       &Data.v[2][0], &Data.v[2][1], &Data.v[2][2],
	       datname) != 15) 
    {
      if ((sscanf(nonwhite,"%d FILE %s", &i, datname) == 2) && (i == 0))
      {
	isMPD++;
      }
      fputs(line,outfile); // Copy line to output file.
    }
    else if (Data.LineType != 1)
    {
      // Gotta handle other linetypes
      fputs(line,outfile); // Copy line to output file.
    }
    else if (isMPD > 1)
    {
      // Only mirror the main (first) file in an MPD.
      fputs(line,outfile); // Copy line to output file.
    }
    else
    {
      strupper(datname);

      for( i=0; i< CategorySize[0]; i++ )
	if (!stricmp(datname, CategoryPtr[0][i].DatName ))
	{
	  // DatName appears in mirror.ini.  Use the matrix from there.
	  me = CategoryPtr[0][i].FirstLine->v;
	  // Also use the substitute part
	  strcpy(datname, CategoryPtr[0][i].FirstLine->Comment);
	  break;
	}

      M4M4Mul(m,LinePtr->v,me); // Run initial mirror on part
      M4M4Mul(LinePtr->v,mp,m); // Apply the global mirror to part

      //Round very small numbers to 0.0
      memcpy(m, LinePtr->v, sizeof(LinePtr->v));
      for (k = 0; k < 4; k++)
	for (j = 0; j < 4; j++)
	{
	  if (fabs(m[k][j]) < 0.000001)
	    m[k][j] = 0.0;
	}

      fprintf(outfile,"%d %d %g %g %g %g %g %g %g %g %g %g %g %g %s\n",
	      LinePtr->LineType, LinePtr->Color,
	      m[0][3],m[1][3],m[2][3],
	      m[0][0],m[0][1],m[0][2],
	      m[1][0],m[1][1],m[1][2],
	      m[2][0],m[2][1],m[2][2],
	      datname);
    }
  }
  fclose(dat);
  fclose(outfile);

  printf("MirWiz complete\n");
  return 1;
}



