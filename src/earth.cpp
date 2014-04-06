/*
 * Compiz Earth plugin
 *
 * earth.cpp
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

#include <earth/earth.h>
#include <glibmm/miscutils.h>

COMPIZ_PLUGIN_20090315 (earth, EarthPluginVTable)

static CompString pname = "earth";

void EarthScreen::optionChange (CompOption *option, Options num)
{
    switch (num)
    {
	case EarthOptions::Shaders:
		if (option->value().b())
		{
			Light[EARTH].specular[0] = 0.5;
			Light[EARTH].specular[1] = 0.5;
			Light[EARTH].specular[2] = 0.4;
			Light[EARTH].specular[3] = 1;
		}
		else
		{
			Light[EARTH].specular[0] = 0;
			Light[EARTH].specular[1] = 0;
			Light[EARTH].specular[2] = 0;
			Light[EARTH].specular[3] = 0;
		}
		break;
	default:
		break;
    }
    updateTime = optionGetCloudUpdateTime();
}

void EarthScreen::preparePaint (int ms)
{
    time_t timer = time (NULL);
    struct tm* currenttime = localtime (&timer);
    
    struct stat attrib;
    int res;
    
    /* Earth and Sun positions calculations */
    dec = 23.4400f * cos((6.2831f/365.0000f)*((float)currenttime->tm_yday+10.0000f));
    gha = (float)currenttime->tm_hour-(optionGetTimezone() + (float)currenttime->tm_isdst) + (float)currenttime->tm_min/60.0000f;
    
    /* Realtime cloudmap */
    res = stat (cloudsfile.filename.c_str(), &attrib);
    if (((difftime (timer, attrib.st_mtime) > (3600 * updateTime)) || (res != 0)) && (cloudsthreaddata.started == 0) && optionGetClouds())
    {
	cloudsthreaddata.s = screen;
		//DownloadClouds_t(&cloudsthreaddata);
	pthread_create (&cloudsthreaddata.tid, NULL, &DownloadClouds_t, (void*) &cloudsthreaddata);
    }
    
    if (cloudsthreaddata.finished == 1)
    {
	pthread_join (cloudsthreaddata.tid, NULL);
	TransformClouds (screen);
	CompString imageFileName="clouds.png", pname="earth";
	CompSize mSkySize(128, 128);
	tex[CLOUDS] = GLTexture::readImageToTexture(imageFileName,pname,mSkySize);
	cloudsthreaddata.finished = 0;
	cloudsthreaddata.started = 0;
    }
    
    cScreen->preparePaint (ms);
}

void EarthScreen::cubePaintInside (const GLScreenPaintAttrib &sAttrib, const GLMatrix &transform, CompOutput *output, int size, const GLVector& vector)
{
    if(cubeScreen->getOption("in")->value().b())
	{
		cubeScreen->cubePaintInside (sAttrib, transform, output, size, vector);
		return;
	}
    GLScreenPaintAttrib sA=sAttrib;
    GLMatrix sTransform = transform;
    sA.yRotate += cubeScreen->invert() * 360.0f / size * (cubeScreen->xRotations() - screen->vp().x() * cubeScreen->nOutput());
    gScreen->glApplyTransform(sA, output, &sTransform);
    float ratio = (float)output->height() / output->width();
    if (cubeScreen->multioutputMode() == CubeScreen::Automatic)
		ratio = (float)screen->height() / screen->width();
    glPushMatrix();
    glLoadMatrixf (sTransform.getMatrix());
    glTranslatef (cubeScreen->outputXOffset(), -cubeScreen->outputYOffset(), 0.0f);
    glScalef (cubeScreen->outputXScale(), cubeScreen->outputYScale(), 1.0f);
    // Pushing all the attribs I'm about to modify
    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT);
    glEnable (GL_DEPTH_TEST); 
    glPushMatrix();
    // Actual display
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT1);
    glEnable (GL_BLEND);
    glDisable (GL_COLOR_MATERIAL);
    glScalef (ratio*optionGetEarthSize(),1.0f*optionGetEarthSize(),ratio*optionGetEarthSize());
	//double x=1.0/ratio/optionGetEarthSize(),y=1.0/optionGetEarthSize();
    //glOrtho (-x,x, y,-y, -x,x);
    PreviousOutput = output->id();
    
    // Earth position according to longitude and latitude
    glRotatef ((optionGetSouth()?-1:1)*optionGetLatitude()-90, 1, 0, 0);
    glRotatef ((optionGetSouth()?-1:1)*optionGetLongitude(), 0, 0, 1);
	glRotatef (optionGetSouth()*180, 0, 1 , 0);

    glPushMatrix ();
    
	// Sun position (hour*15 for degree)
	glRotatef (-gha*15, 0, 0, 1);
	glRotatef (-dec, 1, 0, 0);

	glLightfv (GL_LIGHT1, GL_POSITION, Light[SUN].position);
	glLightfv (GL_LIGHT1, GL_AMBIENT, Light[SUN].ambient);
	glLightfv (GL_LIGHT1, GL_DIFFUSE, Light[SUN].diffuse);
	glLightfv (GL_LIGHT1, GL_SPECULAR, Light[SUN].specular);
	
    glPopMatrix ();
    
    // Earth display
    glBlendFunc (GL_ONE, GL_ZERO);
    
    glMaterialfv(GL_FRONT, GL_AMBIENT, Light[EARTH].ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, Light[EARTH].diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, Light[EARTH].specular);
    glMaterialf(GL_FRONT, GL_SHININESS, Light[EARTH].shininess);

	for(uint i=0;i<tex[0].size();i++)
	{
    if (shadersupport && optionGetShaders())
    {
	glUseProgram(prog[EARTH]);
	
	glActiveTexture (GL_TEXTURE0);
	tex[DAY][i]->enable(GLTexture::Good);
	
	glActiveTexture (GL_TEXTURE1);
	tex[NIGHT][i]->enable(GLTexture::Good);
	// Pass the textures to the shader
        texloc[DAY] = glGetUniformLocation (prog[EARTH], "daytex");
        texloc[NIGHT] = glGetUniformLocation (prog[EARTH], "nighttex");
        glUniform1i (texloc[DAY], 0);
        glUniform1i (texloc[NIGHT], 1);
    }
    else
	tex[DAY][i]->enable(GLTexture::Good);
	
    glCallList (list[EARTH]);

    if (shadersupport && optionGetShaders())
    {
	glUseProgram(0);
	tex[NIGHT][i]->disable();
	glActiveTexture (GL_TEXTURE0);
    tex[DAY][i]->disable();
    }
    else
    tex[DAY][i]->disable();
	}
	
    // Clouds display
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glMaterialfv(GL_FRONT, GL_SPECULAR, Light[CLOUDS].specular);
	foreach(GLTexture* t, tex[CLOUDS])
	{
		t->enable(GLTexture::Good);
		glCallList (list[CLOUDS]);
		t->disable();
	}
    
    glDisable (GL_LIGHT1);

    // Restore previous state
    glPopMatrix ();

    glPopAttrib();
	glPopMatrix();
    
    damage = TRUE;

    cubeScreen->cubePaintInside (sAttrib, transform, output, size, vector);
}

void EarthScreen::cubeClearTargetOutput (float xRotate, float vRotate)
{
    if(cubeScreen->getOption("in")->value().b())
	{
		cubeScreen->cubeClearTargetOutput (xRotate, vRotate);
		GLTexture* t = tex[SKY].front();if(false)
		{
			//glEnable(GL_BLEND);
			glColor4f(0,0,1,0.5);
			t->enable(GLTexture::Good);
			glCallList (list[SKY]);
			t->disable();
			//glDisable(GL_BLEND);
		}
		return;
	}
    glDisable (GL_LIGHTING);
    
    glPushMatrix();
    
    CompOutput* currentoutput = &screen->outputDevs()[(PreviousOutput+1)%screen->outputDevs().size()];
    
    float ratio = (float)currentoutput->height() / (float)currentoutput->width();
    
    if (cubeScreen->multioutputMode() == CubeScreen::OneBigCube)
	ratio = (float) screen->height() / (float) screen->width();
    
    glScalef (ratio, 1.0f, ratio);
    
    /* Rotate the skydome according to the mouse and the rotation of the Earth */
    glRotatef (vRotate - 90, 1.0f, 0.0f, 0.0f);
    glRotatef (xRotate, 0.0f, 0.0f, 1.0f);
    glRotatef (optionGetLatitude(), 1, 0, 0);
    glRotatef (optionGetLongitude() + 180, 0, 0, 1);

	foreach(GLTexture* t, tex[SKY])
	{
		t->enable(GLTexture::Good);
		glCallList (list[SKY]);
		t->disable();
	}

    /* Now rotate to the position of the sun */
    glRotatef (-gha*15, 0, 0, 1);
    glRotatef (dec, 1, 0, 0);
    
    glTranslatef (0, -5, 0);
    glCallList (list[SUN]);
    
    glPopMatrix();
    
    glEnable (GL_LIGHTING);
    
	
    /* Override CubeClearTargetOutput, as it will either draw its own skydome, or blank the screen
    cubeScreen->cubeClearTargetOutput (xRotate, vRotate);*/

    glClear (GL_DEPTH_BUFFER_BIT);
}

void EarthScreen::donePaint ()
{
    if (damage)
    {
	cScreen->damageScreen();
	damage = FALSE;
    }
    
	cScreen->donePaint();
}

EarthScreen::EarthScreen (CompScreen *s) :
	PluginClassHandler< EarthScreen, CompScreen >(s),
	screen(s),
	cScreen(CompositeScreen::get(s)),
	gScreen(GLScreen::get(s)),
	cubeScreen(CubeScreen::get(s)),
	damage(false)
{
	ScreenInterface::setHandler(screen);
	CompositeScreenInterface::setHandler(cScreen);
	GLScreenInterface::setHandler(gScreen);
	CubeScreenInterface::setHandler(cubeScreen);

    for (int i=0; i<4; i++)
		TexThreadData[i].base=this;
	cloudsfile.base=this;
	cloudsthreaddata.base=this;
	
    /* Starting the texture images loading threads */
    for (int i=0; i<4; i++)
    {
	TexThreadData[i].s = s;
	TexThreadData[i].num = i;
	//pthread_create (&TexThreadData[i].tid, NULL, &loadTexture, &TexThreadData[i]);
    }
    
    /* cloudsfile initialization */
    cloudsfile.filename = Glib::getenv("HOME") + "/.compiz-1/earth/images/clouds.jpg";
    cloudsfile.stream = NULL;
    cloudsthreaddata.started = 0;
    cloudsthreaddata.finished = 0;
    
    /* cURL initialization */
    curl_global_init (CURL_GLOBAL_DEFAULT);
    curlhandle = curl_easy_init();
    
    if (curlhandle)
    {
	curl_easy_setopt (curlhandle, CURLOPT_URL, "http://home.megapass.co.kr/~holywatr/cloud_data/clouds_2048.jpg");
	curl_easy_setopt (curlhandle, CURLOPT_WRITEFUNCTION, writecloudsfile);
        curl_easy_setopt (curlhandle, CURLOPT_WRITEDATA, &cloudsfile);
    }
    
    /* Load the shaders */
    createShaders();
    
    /* Lights and materials settings */
    for (int i=0; i<4; i++)
    {
	Light[SUN].ambient[i] = 0.2;
	Light[SUN].diffuse[i] = 1;
	Light[SUN].specular[i] = 1;
	Light[SUN].position[i] = 0;
	
	Light[EARTH].ambient[i] = 0.1;
	Light[EARTH].diffuse[i] = 1;
	Light[EARTH].specular[i] = 0;
	
	Light[CLOUDS].specular[i] = 0;
    }
    Light[SUN].position[1] = 1;
    
    Light[EARTH].shininess = 50.0;
    
    /* Display lists creation */
    createLists ();
    
    /* Join the texture images loading threads, bind the images to actual textures and free the images data */
    for (int i=0; i<4; i++)
    {
	//pthread_join (TexThreadData[i].tid, NULL);
	//tex[i]=GLTexture::imageBufferToTexture((char*)imagedata[i].image, imagedata[i].size);
	//free(imagedata[i].image);
    }
	for(int i=0;i<4;i++)loadTexture(&TexThreadData[i]);
    ChangeNotify optionC = boost::bind(&EarthScreen::optionChange,this,_1,_2);
	
    /* BCOP */
    optionSetLatitudeNotify (optionC);
    optionSetLongitudeNotify (optionC);
    optionSetShadersNotify (optionC);
	optionSetCloudUpdateTimeNotify (optionC);
    optionChange (NULL,(Options)NULL);
}

EarthScreen::~EarthScreen ()
{
    /* Free display list */
    glDeleteLists (list[0], 4);

    // /* Free textures data */
    //for (int i=0; i<4; i++)
	//	foreach(GLTexture t,tex[i])
	//		t.destroyTexture ();
    
    /* Detach and free shaders */
    deleteShaders ();
    
    /* cURL cleanup */
    if (curlhandle)
	curl_easy_cleanup (curlhandle);
    curl_global_cleanup ();
}

void EarthScreen::makeSphere (GLdouble radius, GLboolean inside)
{
    GLfloat sinCache1a[65];
    GLfloat cosCache1a[65];
    GLfloat sinCache2a[65];
    GLfloat cosCache2a[65];
    GLfloat sinCache1b[65];
    GLfloat cosCache1b[65];
    GLfloat sinCache2b[65];
    GLfloat cosCache2b[65];
    GLfloat angle;
    GLfloat zLow, zHigh;
    GLfloat sintemp1 = 0.0, sintemp2 = 0.0, sintemp3 = 0.0, sintemp4 = 0.0;
    GLfloat costemp3 = 0.0, costemp4 = 0.0;

    for (GLint i = 0; i < 64; i++)
    {
	angle = 2* M_PI * i / 64;
	sinCache1a[i] = sinf (angle);
	cosCache1a[i] = cosf (angle);
	sinCache2a[i] = sinCache1a[i];
	cosCache2a[i] = cosCache1a[i];
    }

    for (GLint j = 0; j <= 64; j++)
    {
	angle = M_PI * j / 64;
	if (!inside)
	{
	    sinCache2b[j] = sinf (angle);
	    cosCache2b[j] = cosf (angle);
	}
	else
	{
	    sinCache2b[j] = -sinf (angle);
	    cosCache2b[j] = -cosf (angle);
	}
	sinCache1b[j] = radius * sinf (angle);
	cosCache1b[j] = radius * cosf (angle);
    }
    sinCache1b[0] = 0;
    sinCache1b[64] = 0;

    sinCache1a[64] = sinCache1a[0];
    cosCache1a[64] = cosCache1a[0];
    sinCache2a[64] = sinCache2a[0];
    cosCache2a[64] = cosCache2a[0];

    for (GLint j = 0; j < 64; j++)
    {
        zLow = cosCache1b[j];
        zHigh = cosCache1b[j+1];
        
        sintemp1 = sinCache1b[j];
        sintemp2 = sinCache1b[j+1];
        
	if (!inside)
	{
	    sintemp3 = sinCache2b[j+1];
	    costemp3 = cosCache2b[j+1];
	    sintemp4 = sinCache2b[j];
	    costemp4 = cosCache2b[j];
	}
	else
	{
	    sintemp3 = sinCache2b[j];
	    costemp3 = cosCache2b[j];
	    sintemp4 = sinCache2b[j+1];
	    costemp4 = cosCache2b[j+1];
	}
        glBegin (GL_QUAD_STRIP);
	    for (int i = 0; i <= 64; i++)
	    {
		glNormal3f(sinCache2a[i] * sintemp3, cosCache2a[i] * sintemp3, costemp3);
		if (!inside)
		{
		    glTexCoord2f(1-(float) i / 64, (float) (j+1) / 64);
		    glVertex3f(sintemp2 * sinCache1a[i], sintemp2 * cosCache1a[i], zHigh);
		}
		else
		{
		    glTexCoord2f(1-(float) i / 64, (float) j / 64);
		    glVertex3f(sintemp1 * sinCache1a[i], sintemp1 * cosCache1a[i], zLow);
		}
    	    
		glNormal3f(sinCache2a[i] * sintemp4, cosCache2a[i] * sintemp4, costemp4);
		if (!inside)
		{
		    glTexCoord2f(1-(float) i / 64, (float) j / 64);
		    glVertex3f(sintemp1 * sinCache1a[i], sintemp1 * cosCache1a[i], zLow);
		}
		else
		{
		    glTexCoord2f(1-(float) i / 64, (float) (j+1) / 64);
		    glVertex3f(sintemp2 * sinCache1a[i], sintemp2 * cosCache1a[i], zHigh);
		}
	    }
        glEnd ();
    }
}

CompString LoadSource (const char* filename)
{
    CompString src;   /* shader source code */
    std::ifstream fi(filename);    /* file */
    if (!fi.is_open())
    {
	compLogMessage("earth",CompLogLevelError,"unable to load '%s'", filename);
        return NULL;
    }
    while(fi)
		fi>>src;
	fi.close();
    return src;
}

void* loadTexture (void* p)
{
	EarthScreen::_TexThreadData* threaddata = (EarthScreen::_TexThreadData*)p;
    int num = threaddata->num;
    
    CompString texfile=getenv("HOME");
	texfile+="/.compiz-1/earth/images/";
    
    switch (num)
    {
		   case DAY:	texfile+="day.png";	break;
		   case NIGHT:	texfile+="night.png";	break;
		   case SKY:	texfile+="skydome.png";	break;
		   case CLOUDS:	texfile+="clouds.png";	break;
    }
    threaddata->base->tex[num] = GLTexture::readImageToTexture(texfile,pname,threaddata->base->imagedata[num].size);
    return NULL;
}

void* DownloadClouds_t (void* threaddata)
{
   EarthScreen:: CloudsThreadData* data = (EarthScreen::CloudsThreadData*) threaddata;
    //EARTH_SCREEN (data->s);
    
    data->started = 1;
    data->finished = 0;
    
    /* cloudsfile initialization */
    data->base->cloudsfile.stream = NULL;
    
    /* Download the jpg file */
    if (data->base->curlhandle)
	curl_easy_perform (data->base->curlhandle);
    
    if (data->base->cloudsfile.stream)
	fclose(data->base->cloudsfile.stream);
    
    data->finished = 1;
    return NULL;
}

void EarthScreen::createShaders ()
{
    /* Shader support */
    glewInit ();
    if (glewIsSupported ("GL_VERSION_2_0"))
	shadersupport = GL_TRUE;
    
    if (shadersupport)
    {
	/* Shader creation, loading and compiling */
	vert[EARTH] = glCreateShader (GL_VERTEX_SHADER);
	frag[EARTH] = glCreateShader (GL_FRAGMENT_SHADER);
	
	vertfile[EARTH] += Glib::getenv("HOME") + "/.compiz-1/earth/data/earth.vert";
	fragfile[EARTH] += Glib::getenv("HOME") + "/.compiz-1/earth/data/earth.frag";
		
	vertsource[EARTH] = LoadSource (vertfile[EARTH].c_str());
	fragsource[EARTH] = LoadSource (fragfile[EARTH].c_str());

	const char*c=vertsource[EARTH].c_str();
	glShaderSource (vert[EARTH], 1, &c, NULL);
	c=fragsource[EARTH].c_str();
	glShaderSource (frag[EARTH], 1, &c, NULL);
	
	
	glCompileShader (vert[EARTH]);
	glCompileShader (frag[EARTH]);
	
	/* Program creation, attaching and linking */
	prog[EARTH] = glCreateProgram ();
	
	glAttachShader (prog[EARTH], vert[EARTH]);
	glAttachShader (prog[EARTH], frag[EARTH]);
	
	glLinkProgram (prog[EARTH]);
    }
}

void EarthScreen::deleteShaders ()
{
    if (shadersupport)
    {
	glDetachShader(prog[EARTH], vert[EARTH]);
	glDetachShader(prog[EARTH], frag[EARTH]);
	
	glDeleteShader(vert[EARTH]);
	glDeleteShader(frag[EARTH]);
	
	glDeleteProgram(prog[EARTH]);
    }
}

void EarthScreen::createLists ()
{
    int i;
    list[0] = glGenLists (4);
    
    for (i=0; i<4; i++)
    {
	GLdouble radius;
	GLboolean inside;
	
	switch (i)
	{
	    case SUN:	    radius = 0.1;	inside = TRUE;	break;
	    case EARTH:	    radius = 0.89;	inside = FALSE;	break;
	    case CLOUDS:	radius = 0.9;	inside = FALSE;	break;
	    case SKY:	    radius = 10;	inside = TRUE;	break;
	}
	
	list[i] = list[0] + i;
	//if(cubeScreen->getOption("in")->value().b() && (i == EARTH || i == CLOUDS)) {radius = 3 - radius ;inside=true;}
	glNewList (list[i], GL_COMPILE);
	    makeSphere (radius, inside);
	glEndList ();
    }
}

static size_t writecloudsfile(void *buffer, size_t size, size_t nmemb, void *stream)
{
    EarthScreen::CloudsFile* out = (EarthScreen::CloudsFile*) stream;
    if (out && !out->stream)
    {
	/* open file for writing */ 
	out->stream = fopen (out->filename.c_str(), "wb");
	
	if(!out->stream)
	    return -1; /* failure, can't open file to write */ 
  }
  return fwrite(buffer, size, nmemb, out->stream);
}

void TransformClouds (CompScreen* s)
{
    void* jpgdata;
    int h, w;
    CompString imagefile = "clouds.jpg";
	CompString pname="earth";
	CompSize psize;
    if(!s->readImageFromFile (imagefile, pname, psize, jpgdata))
		return;
    int width=psize.width();
	int height=psize.height();
    char* p_jpgdata = (char*) jpgdata;
    /* Adjust alpha channel */
    for (h=0; h<height; h++)
    {
        for (w=0; w<width; w++)
        {
	    int pos = h * width + w;
	    #if __BYTE_ORDER == __BIG_ENDIAN
	    p_jpgdata[(pos * 4) + 0] = p_jpgdata[(pos * 4) + 1];
	    #else
	    p_jpgdata[(pos * 4) + 3] = p_jpgdata[(pos * 4) + 1];
	    #endif
        }
    }
    
    /* Flip image vertically */
    char* p_pngdata = new char[ width * height * 4 ];
    
    for (int h = 0; h < height; h++)
		for(int i = 0; i < width * 4; i++)
			p_pngdata[h * width * 4 + i] = p_jpgdata[(height - h - 1) * width * 4 + i];

    free (jpgdata);
    
    // Write in the pngfile
	CompString imagefile2 = getenv ("HOME");
	imagefile2 += "/.compiz-1/earth/images/clouds.png";
    s->writeImageToFile (imagefile2, "png", psize, p_pngdata);
    
    /* Clean */
    delete[] p_pngdata;
}

bool EarthPluginVTable::init()
{
	if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
		return false;
	if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
		return false;
	if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
		return false;
	if (!CompPlugin::checkPluginABI ("cube", COMPIZ_CUBE_ABI))
		return false;
	return true;
}
