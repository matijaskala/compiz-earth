#ifndef __EARTH_H__
#define __EARTH_H__

#define _GNU_SOURCE

#include <math.h>
#include <float.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <compiz-core.h>
#include <compiz-cube.h>

extern int earthDisplayPrivateIndex;
extern int cubeDisplayPrivateIndex;

#define GET_EARTH_DISPLAY(d) \
    ((EarthDisplay *) (d)->base.privates[earthDisplayPrivateIndex].ptr)
#define EARTH_DISPLAY(d) \
    EarthDisplay *ed = GET_EARTH_DISPLAY(d);

#define GET_EARTH_SCREEN(s, ed) \
    ((EarthScreen *) (s)->base.privates[(ed)->screenPrivateIndex].ptr)
#define EARTH_SCREEN(s) \
    EarthScreen *es = GET_EARTH_SCREEN(s, GET_EARTH_DISPLAY(s->display))

typedef struct _EarthDisplay
{
    int screenPrivateIndex;
} EarthDisplay;

typedef struct _LightParam
{
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat position[4];
    GLfloat emission[4];
    GLfloat shininess;
} LightParam;

typedef struct _EarthScreen
{
    DonePaintScreenProc    donePaintScreen;
    PreparePaintScreenProc preparePaintScreen;

    CubeClearTargetOutputProc clearTargetOutput;
    CubePaintInsideProc       paintInside;

    Bool damage;
    
    /* Time */
    time_t timer;
    struct tm * temps;
    
    /* Config parameters */
    float lon, lat;
    float tz;
    
    /* Sun position */
    float dec, gha;
    
    /* Textures */
    char* datapath;
    char* daytexfile;
    char* nighttexfile;
    CompTexture* daytex;
    CompTexture* nighttex;
    
    /* Rendering */
    LightParam sun, earth;
    GLuint spherelist;
    
    /* Shaders */
    GLboolean shadersupport;
    char* earthvertfile;
    char* earthvertsource;
    char* earthfragfile;
    char* earthfragsource;
    GLuint earthvert, earthfrag, earthprog;
    GLint daytexloc, nighttexloc;
    
} EarthScreen;

void makeSphere (GLdouble radius);
char* LoadSource (char *filename);

#endif
