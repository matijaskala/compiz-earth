#ifndef __EARTH_H__
#define __EARTH_H__

#define _GNU_SOURCE

#include <math.h>
#include <float.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <curl/curl.h>

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
    
#define DAY	0
#define NIGHT	1
#define CLOUDS	2
#define SKY	3

#define EARTH	0
#define SUN	1


typedef struct _LightParam
{
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat position[4];
    GLfloat emission[4];
    GLfloat shininess;
} LightParam;

typedef struct _texthreaddata
{
    CompScreen* s;
    int num;
    pthread_t tid;
} TexThreadData;

typedef struct _cloudsthreaddata
{
    CompScreen* s;
    pthread_t tid;
    int started;
    int finished;
} CloudsThreadData;

typedef struct _imagedata
{
    void* image;
    int width, height;
} ImageData;

typedef struct _cloudsFile
{
    char* filename;
    FILE* stream;
} CloudsFile;

typedef struct _EarthDisplay
{
    int screenPrivateIndex;
} EarthDisplay;

typedef struct _EarthScreen
{
    DonePaintScreenProc    donePaintScreen;
    PreparePaintScreenProc preparePaintScreen;

    CubeClearTargetOutputProc clearTargetOutput;
    CubePaintInsideProc       paintInside;

    Bool damage;
    
    /* Config parameters */
    float lon, lat;
    float tz;
    Bool  shaders;
    Bool  clouds;
    Bool south_on_top;
    float earth_size;
    
    int previousoutput;
    
    /* Sun position */
    float dec, gha;
    
    /* Threads */
    TexThreadData texthreaddata [4];
    CloudsThreadData cloudsthreaddata;
    
    /* Clouds */
    CURL* curlhandle;
    CloudsFile cloudsfile;
    
    /* Textures */
    ImageData imagedata [4];
    CompTexture* tex [4];
    
    /* Rendering */
    LightParam light [3];
    GLuint list [4];
    
    /* Shaders */
    GLboolean shadersupport;
    char* vertfile [1];
    char* vertsource [1];
    char* fragfile [1];
    char* fragsource [1];
    GLuint vert [1];
    GLuint frag [1];
    GLuint prog [1];
    GLint texloc [2];
    
} EarthScreen;

void makeSphere (GLdouble radius, GLboolean inside);
char* LoadSource (char *filename);
void* LoadTexture_t (void* threaddata);
void CreateShaders (EarthScreen* es);
void DeleteShaders (EarthScreen* es);
void CreateLists (EarthScreen* es);
static size_t writecloudsfile(void *buffer, size_t size, size_t nmemb, void *stream);
void* DownloadClouds_t (void* threaddata);
void TransformClouds (CompScreen* s);

#endif
