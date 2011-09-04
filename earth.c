#include "earth.h"
#include "earth_options.h"

int earthDisplayPrivateIndex;
int cubeDisplayPrivateIndex;

static void
earthScreenOptionChanged (CompScreen		*s,
			  CompOption		*opt,
			  EarthScreenOptions	num)
{
    EARTH_SCREEN (s);
    switch (num)
    {
	case EarthScreenOptionLatitude:	    es->lat = earthGetLatitude (s);	break;
	case EarthScreenOptionLongitude:    es->lon = earthGetLongitude (s);	break;
	case EarthScreenOptionTimezone:	    es->tz  = earthGetTimezone (s);	break;
	
	case EarthScreenOptionShaders:
	    es->shaders = earthGetShaders (s);
	    if (opt->value.b == TRUE)
	    {
	    	es->light[EARTH].specular[0] = 0.5;
		es->light[EARTH].specular[1] = 0.5;
		es->light[EARTH].specular[2] = 0.4;
		es->light[EARTH].specular[3] = 1;
	    }
	    else
	    {
	    	es->light[EARTH].specular[0] = 0;
		es->light[EARTH].specular[1] = 0;
		es->light[EARTH].specular[2] = 0;
		es->light[EARTH].specular[3] = 0;
	    }
	break;
	
	default:								break;
    }
}

static void
earthPreparePaintScreen (CompScreen *s,
			 int        ms)
{
    EARTH_SCREEN (s);
    
    /* Earth and Sun positions calculations */
    es->timer = time (NULL);
    es->temps = localtime (&es->timer);
    es->dec = 23.45f * cos((6.2831f/365.0f)*((float)es->temps->tm_yday+10.0f));
    es->gha = (float)es->temps->tm_hour-(es->tz + (float)es->temps->tm_isdst) + (float)es->temps->tm_min/60.00f;

    UNWRAP (es, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (es, s, preparePaintScreen, earthPreparePaintScreen);
}

static void
earthPaintInside (CompScreen              *s,
		  const ScreenPaintAttrib *sAttrib,
		  const CompTransform     *transform,
		  CompOutput              *output,
		  int                     size)
{
    EARTH_SCREEN (s);
    CUBE_SCREEN (s);

    ScreenPaintAttrib sA = *sAttrib;

    sA.yRotate += cs->invert * (360.0f / size) *
		  (cs->xRotations - (s->x * cs->nOutput));

    CompTransform mT = *transform;

    (*s->applyScreenTransform) (s, &sA, output, &mT);

    glPushMatrix();
    glLoadMatrixf (mT.m);
    glTranslatef (cs->outputXOffset, -cs->outputYOffset, 0.0f);
    glScalef (cs->outputXScale, cs->outputYScale, 1.0f);

    /* Pushing all the attribs I'm about to modify*/
    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT |GL_ENABLE_BIT);

    glEnable (GL_DEPTH_TEST); 

    glPushMatrix();
	
    /* Actual display */
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT1);
    glEnable (GL_BLEND);
    glDisable (GL_COLOR_MATERIAL);
    
    float ratio = (float)s->height / (float)s->width;
    glScalef (ratio, 1.0f, ratio);
    
    /* Earth position according to longitude and latitude */
    glRotatef (es->lat-90, 1, 0, 0);
    glRotatef (es->lon, 0, 0, 1);
    
    glPushMatrix ();
    
	/* Sun position (hour*15 for degree) */
	glRotatef (-es->gha*15, 0, 0, 1);
	glRotatef (-es->dec, 1, 0, 0);

	glLightfv (GL_LIGHT1, GL_POSITION, es->light[SUN].position);
	glLightfv (GL_LIGHT1, GL_AMBIENT, es->light[SUN].ambient);
	glLightfv (GL_LIGHT1, GL_DIFFUSE, es->light[SUN].diffuse);
	glLightfv (GL_LIGHT1, GL_SPECULAR, es->light[SUN].specular);
	
    glPopMatrix ();
    
    /* Earth display */
    glBlendFunc (GL_ONE, GL_ZERO);
    
    glMaterialfv(GL_FRONT, GL_AMBIENT, es->light[EARTH].ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, es->light[EARTH].diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, es->light[EARTH].specular);
    glMaterialf(GL_FRONT, GL_SHININESS, es->light[EARTH].shininess);
    
    if (es->shadersupport && es->shaders)
    {
	glUseProgram(es->prog[EARTH]);
	
	glActiveTexture (GL_TEXTURE0);
	enableTexture (s, es->tex[DAY], COMP_TEXTURE_FILTER_GOOD);
	
	glActiveTexture (GL_TEXTURE1);
	enableTexture (s, es->tex[NIGHT], COMP_TEXTURE_FILTER_GOOD);
	
	/* Pass the textures to the shader */
        es->texloc[DAY] = glGetUniformLocation (es->prog[EARTH], "daytex");
        es->texloc[NIGHT] = glGetUniformLocation (es->prog[EARTH], "nighttex");
	
        glUniform1i (es->texloc[DAY], 0);
        glUniform1i (es->texloc[NIGHT], 1);
    }
    else
	enableTexture (s, es->tex[DAY], COMP_TEXTURE_FILTER_GOOD);
	
	
    glCallList (es->list[EARTH]);
    
    
    if (es->shadersupport && es->shaders)
    {
	glUseProgram(0);
	disableTexture (s, es->tex[NIGHT]);
	glActiveTexture (GL_TEXTURE0);
	disableTexture (s, es->tex[DAY]);
    }
    else
	disableTexture (s, es->tex[DAY]);
	
    // Clouds display
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glMaterialfv(GL_FRONT, GL_SPECULAR, es->light[CLOUDS].specular);    
    enableTexture (s, es->tex[CLOUDS], COMP_TEXTURE_FILTER_GOOD);
	glCallList (es->list[CLOUDS]);
    disableTexture (s, es->tex[CLOUDS]);
    
    
    glDisable (GL_LIGHT1);

    /* Restore previous state */
    glPopMatrix ();

    glPopAttrib();
	glPopMatrix();
    
    es->damage = TRUE;

    UNWRAP (es, cs, paintInside);
    (*cs->paintInside) (s, sAttrib, transform, output, size);
    WRAP (es, cs, paintInside, earthPaintInside);
}

static void
earthClearTargetOutput (CompScreen *s,
			float      xRotate,
			float      vRotate)
{
    EARTH_SCREEN (s);
    CUBE_SCREEN (s);
    
    
    glDisable (GL_LIGHTING);  
    
    glPushMatrix();
    
    float ratio = (float)s->height / (float)s->width;
    glScalef (ratio, 1.0f, ratio);
    
    /* Rotate the skydome according to the mouse and the rotation of the Earth */
    glRotatef (vRotate - 90, 1.0f, 0.0f, 0.0f);
    glRotatef (xRotate, 0.0f, 0.0f, 1.0f);
    glRotatef (es->lat, 1, 0, 0);
    glRotatef (es->lon + 180, 0, 0, 1);
    
    enableTexture (s, es->tex[SKY], COMP_TEXTURE_FILTER_GOOD);
    glCallList (es->list[SKY]);
    disableTexture (s, es->tex[SKY]);

    /* Now rotate to the position of the sun */
    glRotatef (-es->gha*15, 0, 0, 1);
    glRotatef (es->dec, 1, 0, 0);
    
    glTranslatef (0, -5, 0);
    glCallList (es->list[SUN]);
    
    glPopMatrix();
    
    glEnable (GL_LIGHTING);
    
	
    UNWRAP (es, cs, clearTargetOutput);
    /* Override CubeClearTargetOutput, as it will either draw its own skydome, or blank the screen
    (*cs->clearTargetOutput) (s, xRotate, vRotate);*/
    WRAP (es, cs, clearTargetOutput, earthClearTargetOutput);

    glClear (GL_DEPTH_BUFFER_BIT);
}

static void
earthDonePaintScreen (CompScreen * s)
{
    EARTH_SCREEN (s);

    if (es->damage)
    {
	damageScreen (s);
	es->damage = FALSE;
    }

    UNWRAP (es, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (es, s, donePaintScreen, earthDonePaintScreen);
}

static Bool
earthInitDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    EarthDisplay *ed;

    if (!checkPluginABI ("core", CORE_ABIVERSION)
	|| !checkPluginABI ("cube", CUBE_ABIVERSION))
	return FALSE;

    if (!getPluginDisplayIndex (d, "cube", &cubeDisplayPrivateIndex))
	return FALSE;

    ed = malloc (sizeof (EarthDisplay));
    if (!ed)
	return FALSE;

    ed->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (ed->screenPrivateIndex < 0)
    {
	free (ed);
	return FALSE;
    }

    d->base.privates[earthDisplayPrivateIndex].ptr = ed;

    return TRUE;
}



static Bool
earthInitScreen (CompPlugin *p,
		CompScreen *s)
{
    EarthScreen *es;

    EARTH_DISPLAY (s->display);
    CUBE_SCREEN (s);
    
    int i;

    es = malloc (sizeof (EarthScreen));
    if (!es)
	return FALSE;

    s->base.privates[ed->screenPrivateIndex].ptr = es;

    es->damage = FALSE;
    
    /* Starting the texture images loading threads */
    for (i=0; i<4; i++)
    {
	es->texthreaddata[i].s = s;
	es->texthreaddata[i].num = i;
	pthread_create (&es->texthreaddata[i].tid, NULL, LoadTexture_t, (void*) &es->texthreaddata[i]);
    }
    
    /* Load the shaders */
    CreateShaders(es);
    
    /* Lights and materials settings */
    for (i=0; i<4; i++)
    {
	es->light[SUN].ambient[i] = 0.2;
	es->light[SUN].diffuse[i] = 1;
	es->light[SUN].specular[i] = 1;
	es->light[SUN].position[i] = 0;
	
	es->light[EARTH].ambient[i] = 0.1;
	es->light[EARTH].diffuse[i] = 1;
	es->light[EARTH].specular[i] = 0;
	
	es->light[CLOUDS].specular[i] = 0;
    }
    es->light[SUN].position[1] = 1;
    
    es->light[EARTH].shininess = 50.0; 
    
    /* Display lists creation */
    CreateLists (es);
    
    /* Join the texture images loading threads, bind the images to actual textures and free the images data */
    for (i=0; i<4; i++)
    {
	pthread_join (es->texthreaddata[i].tid, NULL);
	imageBufferToTexture (s, es->tex[i], es->imagedata[i].image, es->imagedata[i].width, es->imagedata[i].height);
	free (es->imagedata[i].image);
    }
    
    /* BCOP */
    earthSetLatitudeNotify (s, earthScreenOptionChanged);
    earthSetLongitudeNotify (s, earthScreenOptionChanged);
    earthSetTimezoneNotify (s, earthScreenOptionChanged);
    earthSetShadersNotify (s, earthScreenOptionChanged);
    
    earthScreenOptionChanged (s, earthGetShadersOption (s),EarthScreenOptionShaders);
    
    WRAP (es, s, donePaintScreen, earthDonePaintScreen);
    WRAP (es, s, preparePaintScreen, earthPreparePaintScreen);
    WRAP (es, cs, clearTargetOutput, earthClearTargetOutput);
    WRAP (es, cs, paintInside, earthPaintInside);

    return TRUE;
}

static void
earthFiniScreen (CompPlugin *p,
		     CompScreen *s)
{
    EARTH_SCREEN (s);
    CUBE_SCREEN (s);
    
    int i;
    
    /* Free display list */
    glDeleteLists (es->list[0], 4);

    /* Free textures data */
    for (i=0; i<4; i++)
	destroyTexture (s, es->tex[i]);
    
    /* Detach and free shaders */
    DeleteShaders (es);
    
    
    UNWRAP (es, s, donePaintScreen);
    UNWRAP (es, s, preparePaintScreen);
    UNWRAP (es, cs, clearTargetOutput);
    UNWRAP (es, cs, paintInside);

    free (es);
}

static void
earthFiniDisplay (CompPlugin  *p,
		      CompDisplay *d)
{
    EARTH_DISPLAY (d);

    freeScreenPrivateIndex (d, ed->screenPrivateIndex);
    free (ed);
}

static Bool
earthInit (CompPlugin *p)
{
    earthDisplayPrivateIndex = allocateDisplayPrivateIndex ();

    if (earthDisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static CompBool
earthInitObject (CompPlugin *p,
		     CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0,
	(InitPluginObjectProc) earthInitDisplay,
	(InitPluginObjectProc) earthInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
earthFiniObject (CompPlugin *p,
		     CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0,
	(FiniPluginObjectProc) earthFiniDisplay,
	(FiniPluginObjectProc) earthFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static void
earthFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (earthDisplayPrivateIndex);
}

CompPluginVTable earthVTable = {
    "earth",
    0,
    earthInit,
    earthFini,
    earthInitObject,
    earthFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &earthVTable;
}

void makeSphere (GLdouble radius, GLboolean inside)
{
    GLint i,j;
    GLfloat PI = 3.14159265358979323846;
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

    for (i = 0; i < 64; i++)
    {
	angle = 2* PI * i / 64;
	sinCache1a[i] = sinf (angle);
	cosCache1a[i] = cosf (angle);
	sinCache2a[i] = sinCache1a[i];
	cosCache2a[i] = cosCache1a[i];
    }

    for (j = 0; j <= 64; j++)
    {
	angle = PI * j / 64;
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

    for (j = 0; j < 64; j++)
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
	    for (i = 0; i <= 64; i++)
	    {
		glNormal3f(sinCache2a[i] * sintemp3, cosCache2a[i] * sintemp3, costemp3);
		if (!inside)
		{
		    glTexCoord2f(1 - (float) i / 64, 1 - (float) (j+1) / 64);
		    glVertex3f(sintemp2 * sinCache1a[i], sintemp2 * cosCache1a[i], zHigh);
		}
		else
		{
		    glTexCoord2f(1 - (float) i / 64, 1 - (float) j / 64);
		    glVertex3f(sintemp1 * sinCache1a[i], sintemp1 * cosCache1a[i], zLow);
		}
    	    
		glNormal3f(sinCache2a[i] * sintemp4, cosCache2a[i] * sintemp4, costemp4);
		if (!inside)
		{
		    glTexCoord2f(1 - (float) i / 64, 1 - (float) j / 64);
		    glVertex3f(sintemp1 * sinCache1a[i], sintemp1 * cosCache1a[i], zLow);
		}
		else
		{
		    glTexCoord2f(1 - (float) i / 64, 1 - (float) (j+1) / 64);
		    glVertex3f(sintemp2 * sinCache1a[i], sintemp2 * cosCache1a[i], zHigh);
		}
	    }
        glEnd ();
    }
}

char* LoadSource (char* filename)
{
    char* src = NULL;   /* shader source code */
    FILE* fp = NULL;    /* file */
    long size;          /* file size */
    long i;             /* count */
    
    
    fp = fopen (filename, "r");
    if (fp == NULL)
    {
        fprintf (stderr, "unable to load '%s'\n", filename);
        return NULL;
    }
    
    fseek (fp, 0, SEEK_END);
    size = ftell (fp);
    
    rewind (fp);
    
    src = malloc (size+1);
    if (src == NULL)
    {
        fclose (fp);
        fprintf (stderr, "memory allocation error !\n");
        return NULL;
    }
    
    for (i=0; i<size; i++)
        src[i] = fgetc (fp);
    
    src[size] = '\0';
    
    fclose (fp);
    
    return src;
}

void* LoadTexture_t (void* pdata)
{
    texthreaddata* data = (texthreaddata*) pdata;
    CompScreen* s = data->s;
    int num = data->num;
    
    EARTH_SCREEN (s);
    char* texfile;
    
    switch (num)
    {
	case DAY:	asprintf (&texfile, "%s", "day.png");	    break;
	case NIGHT:	asprintf (&texfile, "%s", "night.png");	    break;
	case SKY:	asprintf (&texfile, "%s", "skydome.png");   break;
	case CLOUDS:	asprintf (&texfile, "%s", "clouds.png");    break;
    }
    
    es->tex[num] = createTexture (s);
    readImageFromFile (s->display, texfile, &es->imagedata[num].width, &es->imagedata[num].height, &es->imagedata[num].image);
    
    return NULL;
}

void CreateShaders (EarthScreen* es)
{
    /* Shader support */
    glewInit ();
    if (glewIsSupported ("GL_VERSION_2_0"))
	es->shadersupport = GL_TRUE;
    
    if (es->shadersupport)
    {
	/* Get Compiz data directory */
	asprintf (&es->datapath, "%s%s",getenv("HOME"), "/.compiz/data/");
	
	/* Shader creation, loading and compiling */
	es->vert[EARTH] = glCreateShader (GL_VERTEX_SHADER);
	es->frag[EARTH] = glCreateShader (GL_FRAGMENT_SHADER);
	
	asprintf (&es->vertfile[EARTH], "%s%s", es->datapath, "earth.vert");
	/* Load a different shader according to GLSL version */
	if (glewIsSupported ("GL_VERSION_3_0"))
	    asprintf (&es->fragfile[EARTH], "%s%s", es->datapath, "earth.frag");
	else
	    asprintf (&es->fragfile[EARTH], "%s%s", es->datapath, "earth110.frag");
	
	es->vertsource[EARTH] = LoadSource (es->vertfile[EARTH]);
	es->fragsource[EARTH] = LoadSource (es->fragfile[EARTH]);
	
	glShaderSource (es->vert[EARTH], 1, (const GLchar**)&es->vertsource[EARTH], NULL);
	glShaderSource (es->frag[EARTH], 1, (const GLchar**)&es->fragsource[EARTH], NULL);
	
	
	glCompileShader (es->vert[EARTH]);
	glCompileShader (es->frag[EARTH]);
	
	/* Program creation, attaching and linking */
	es->prog[EARTH] = glCreateProgram ();
	
	glAttachShader (es->prog[EARTH], es->vert[EARTH]);
	glAttachShader (es->prog[EARTH], es->frag[EARTH]);
	
	glLinkProgram (es->prog[EARTH]);
	
	/* Cleanup */
	free (es->vertsource[EARTH]);
	free (es->fragsource[EARTH]);
	free (es->vertfile[EARTH]);
	free (es->fragfile[EARTH]);
	free (es->datapath);
    }
}

void DeleteShaders (EarthScreen* es)
{
    if (es->shadersupport)
    {
	glDetachShader(es->prog[EARTH], es->vert[EARTH]);
	glDetachShader(es->prog[EARTH], es->frag[EARTH]);
	
	glDeleteShader(es->vert[EARTH]);
	glDeleteShader(es->frag[EARTH]);
	
	glDeleteProgram(es->prog[EARTH]);
    }
}

void CreateLists (EarthScreen* es)
{
    int i;
    es->list[0] = glGenLists (4);
    
    for (i=0; i<4; i++)
    {
	GLdouble radius;
	GLboolean inside;
	
	switch (i)
	{
	    case SUN:	    radius = 0.1;	inside = TRUE;	break;
	    case EARTH:	    radius = 0.75;	inside = FALSE;	break;
	    case CLOUDS:    radius = 0.76;	inside = FALSE;	break;
	    case SKY:	    radius = 10;	inside = TRUE;	break;
	}
	
	es->list[i] = es->list[0] + i;
	
	glNewList (es->list[i], GL_COMPILE);
	    makeSphere (radius, inside);
	glEndList ();
    }
}