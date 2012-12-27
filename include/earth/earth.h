/*
 * Compiz Earth plugin
 *
 * earth.h
 *
 * This plugin renders a 3D earth inside the cube
 *
 * Copyright : (C) 2010 by Maxime Wack
 * E-mail    : maximewack(at)free(dot)fr
 *
 * Ported to Compiz 0.9.x
 * Copyright : (C) 2012 Matija Skala <mskala@gmx.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef __EARTH_H__
#define __EARTH_H__

#include <cmath>
#include <fstream>
#include <sys/stat.h>
#include <curl/curl.h>

#include <core/core.h>
#include <GL/glew.h>
#include <cube/cube.h>
#include "earth_options.h"

enum
{
	DAY=0,
	NIGHT=1,
	CLOUDS=2,
	SKY=3
};

enum
{
	EARTH=0,
	SUN=1
};

struct LightParam
{
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat position[4];
    GLfloat emission[4];
    GLfloat shininess;
};
class EarthScreen :
	public ScreenInterface,
	public CompositeScreenInterface,
	public GLScreenInterface,
	public CubeScreenInterface,
	public PluginClassHandler<EarthScreen,CompScreen>,
	public EarthOptions
{
public:
    EarthScreen ( CompScreen *s );
    ~EarthScreen();
	CompScreen		*screen;
	CompositeScreen	*cScreen;
	GLScreen		*gScreen;
	CubeScreen      *cubeScreen;
	//CompOption::Vector& getOptions ();
	//bool setOption (const CompString &name, CompOption::Value &value);
	//void cubeGetRotation (float &x, float &v, float &progress);
	void cubeClearTargetOutput (float, float);
	void cubePaintInside (const GLScreenPaintAttrib&, const GLMatrix&, CompOutput*, int);
	void createShaders ();
	void deleteShaders ();
	void createLists ();
	//void paint(CompOutput::ptrList &outputs, unsigned int);
	bool glPaintOutput();
    //DonePaintScreenProc    donePaintScreen;
    //PreparePaintScreenProc preparePaintScreen;

    //CubeClearTargetOutputProc clearTargetOutput;
    //CubePaintInsideProc       paintInside;
	void optionChange(CompOption*, Options);
    int PreviousOutput;
    LightParam Light [3];
	void preparePaint(int);
	void donePaint();

    Bool damage;
    
    
    /* Sun position */
    float dec, gha;
    

struct _TexThreadData{    CompScreen* s;    int num;    pthread_t tid; EarthScreen* base;};

struct CloudsThreadData{    CompScreen* s;    pthread_t tid;    int started;    int finished; EarthScreen* base;};

struct CloudsFile{    char* filename;    FILE* stream; EarthScreen* base;};
    
    /* Clouds */
    CURL* curlhandle;
    CloudsFile cloudsfile;
	float updateTime;
    
    /* Textures */
	struct { void* image; CompSize size; } imagedata [4];
	//CompSize csize [4];
    GLTexture::List tex [4];

//private:
    /* Threads */
    _TexThreadData TexThreadData [4];
    CloudsThreadData cloudsthreaddata;
	void makeSphere (GLdouble radius, GLboolean inside);
    
    /* Rendering */
    GLuint list [4];
    
    /* Shaders */
    GLboolean shadersupport;
    CompString vertfile[1];
    CompString vertsource[1];
    CompString fragfile[1];
    CompString fragsource[1];
    GLuint vert [1];
    GLuint frag [1];
    GLuint prog [1];
    GLint texloc [2];
};

#define EARTH_SCREEN(s) EarthScreen *es = EarthScreen::get (s);

struct EarthPluginVTable : public CompPlugin::VTableForScreen<EarthScreen>
{
	bool init ();
};

CompString LoadSource (CompString filename);

static size_t writecloudsfile(void *buffer, size_t size, size_t nmemb, void *stream);
void TransformClouds (CompScreen* s);
//EarthDisplay* getEarthDisplay(CompDisplay *d);
//EarthScreen* getEarthScreen(CompScreen *s, EarthDisplay *ed);
	void* DownloadClouds_t (void* threaddata);
	void* loadTexture (void* threaddata);

#endif
