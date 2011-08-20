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
	case EarthScreenOptionLatitude:
	    es->lat = earthGetLatitude (s);
	break;
	case EarthScreenOptionLongitude:
	    es->lon = earthGetLongitude (s);
	break;
	case EarthScreenOptionTimezone:
	    es->tz = earthGetTimezone (s);
	break;
	default:
	break;
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

    Bool enabledCull = FALSE;

    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT);

    glDisable (GL_BLEND);

    if (!glIsEnabled (GL_CULL_FACE) )
    {
	enabledCull = TRUE;
	glEnable (GL_CULL_FACE);
    }

    glEnable (GL_DEPTH_TEST); 

    glPushMatrix();
	
    /* Actual display */
    glEnable (GL_LIGHTING);
    glEnable (GL_LIGHT1);
    glDisable (GL_COLOR_MATERIAL);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    float ratio = (float)s->height / (float)s->width;
    glScalef (ratio, 1.0f, ratio);
    
    /* Earth position according to longitude and latitude */
    glRotatef (es->lat-90, 1, 0, 0);
    glRotatef (es->lon, 0, 0, 1);
    
    glPushMatrix ();
    
	/* Sun position (hour*15 for degree) */
	glRotatef (-es->gha*15, 0, 0, 1);
	glRotatef (-es->dec, 1, 0, 0);

	glLightfv (GL_LIGHT1, GL_POSITION, es->sun.position);
	
    glPopMatrix ();
    
    /* Earth display */
    if (es->shadersupport)
    {
	glUseProgram(es->earthprog);
	
	glActiveTexture (GL_TEXTURE0);
	enableTexture (s, es->daytex, COMP_TEXTURE_FILTER_GOOD);
	
	glActiveTexture (GL_TEXTURE1);
	enableTexture (s, es->nighttex, COMP_TEXTURE_FILTER_GOOD);
	
	/* Pass the textures to the shader */
        es->daytexloc = glGetUniformLocation (es->earthprog, "daytex");
        es->nighttexloc = glGetUniformLocation (es->earthprog, "nighttex");
	
        glUniform1i (es->daytexloc, 0);
        glUniform1i (es->nighttexloc, 1);
    }
    else
	enableTexture (s, es->daytex, COMP_TEXTURE_FILTER_GOOD);
	
    glCallList (es->spherelist);
    
    if (es->shadersupport)
    {
	glUseProgram(0);
	disableTexture (s, es->nighttex);
	glActiveTexture (GL_TEXTURE0);
	disableTexture (s, es->daytex);
    }
    else
	disableTexture (s, es->daytex);
    
    
    glDisable (GL_LIGHT1);
    glLightModeli (GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glEnable (GL_COLOR_MATERIAL);

    /* Back to standard openGL settings */
    glPopMatrix ();
    
    if (!s->lighting)
	glDisable (GL_LIGHTING);

    glDisable (GL_DEPTH_TEST);

    if (enabledCull)
	glDisable (GL_CULL_FACE);

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
    
	
    UNWRAP (es, cs, clearTargetOutput);
    (*cs->clearTargetOutput) (s, xRotate, vRotate);
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
    
    
    /* Get Compiz data directory */
    asprintf (&es->datapath, "%s%s",getenv("HOME"), "/.compiz/data/");
    
    
    /* Texture loading and creation */
    asprintf (&es->daytexfile, "%s%s", es->datapath, "day.png");
    asprintf (&es->nighttexfile, "%s%s", es->datapath, "night.png");

    es->daytex = createTexture (s);
    readImageToTexture (s, es->daytex, es->daytexfile, 0, 0);
    es->nighttex = createTexture (s);
    readImageToTexture (s, es->nighttex, es->nighttexfile, 0, 0);
    
    
    /* Shader support */
    glewInit ();
    if (glewIsSupported ("GL_VERSION_2_0"))
	es->shadersupport = GL_TRUE;
    
    if (es->shadersupport)
    {
	/* Shader creation, loading and compiling */
	es->earthvert = glCreateShader (GL_VERTEX_SHADER);
	es->earthfrag = glCreateShader (GL_FRAGMENT_SHADER);
	
	asprintf (&es->earthvertfile, "%s%s", es->datapath, "earth.vert");
	asprintf (&es->earthfragfile, "%s%s", es->datapath, "earth.frag");
	
	es->earthvertsource = LoadSource (es->earthvertfile);
	es->earthfragsource = LoadSource (es->earthfragfile);
	
	glShaderSource (es->earthvert, 1, (const GLchar**)&es->earthvertsource, NULL);
	glShaderSource (es->earthfrag, 1, (const GLchar**)&es->earthfragsource, NULL);
	
	glCompileShader (es->earthvert);
	glCompileShader (es->earthfrag);
	
	/* Program creation, attaching and linking */
	es->earthprog = glCreateProgram ();
	
	glAttachShader (es->earthprog, es->earthvert);
	glAttachShader (es->earthprog, es->earthfrag);
	
	glLinkProgram (es->earthprog);
	
	/* Cleanup */
	free (es->earthvertsource);
	free (es->earthfragsource);
	free (es->earthvertfile);
	free (es->earthfragfile);
    }
    
    /* Some cleanup */
    free (es->datapath);
    free (es->daytexfile);
    free (es->nighttexfile);
    
    
    /* Lighting and material settings */
    for (i=0; i<4; i++)
    {
	es->sun.ambient[i] = 0.2;
	es->sun.diffuse[i] = 1;
	es->sun.specular[i] = 1;
	es->sun.position[i] = 0;
	
	es->earth.ambient[i] = 0.2;
	es->earth.diffuse[i] = 1;
	es->earth.specular[i] = 0;
    }
    es->sun.position[1] = 1;
    
    if (es->shadersupport) /* Enable specular only with shader support because of the specific treatment in the shader (spec on water only) */
    {
	es->earth.shininess = 50.0;
	es->earth.specular[0] = 0.5;
	es->earth.specular[1] = 0.5;
	es->earth.specular[2] = 0.4;
	es->earth.specular[3] = 1;
    }
    
    glLightfv (GL_LIGHT1, GL_AMBIENT, es->sun.ambient);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, es->sun.diffuse);
    glLightfv (GL_LIGHT1, GL_SPECULAR, es->sun.specular);
    
    /* Display list creation */
    es->spherelist = glGenLists (1);
    
    glNewList (es->spherelist, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT, es->earth.ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, es->earth.diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, es->earth.specular);
	glMaterialf(GL_FRONT, GL_SHININESS, es->earth.shininess);
	makeSphere (0.75);
    glEndList ();
    
    /* BCOP */
    earthSetLatitudeNotify (s, earthScreenOptionChanged);
    earthSetLongitudeNotify (s, earthScreenOptionChanged);
    earthSetTimezoneNotify (s, earthScreenOptionChanged);
    
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
    
    /* Free display list */
    glDeleteLists (es->spherelist, 1);

    /* Free textures data */
    destroyTexture (s, es->nighttex);
    destroyTexture (s, es->daytex);
    
    /* Detach and free shaders */
    if (es->shadersupport)
    {
	glDetachShader(es->earthprog, es->earthvert);
	glDetachShader(es->earthprog, es->earthfrag);
	
	glDeleteShader(es->earthvert);
	glDeleteShader(es->earthfrag);
	
	glDeleteProgram(es->earthprog);
    }
    
    
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

void makeSphere (GLdouble radius)
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
	sinCache2b[j] = sinf (angle); // -SIN si inside
	cosCache2b[j] = cosf (angle);// -COS si inside
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
        
        sintemp3 = sinCache2b[j+1]; // +0 si inside
        costemp3 = cosCache2b[j+1]; // +0 si inside
        sintemp4 = sinCache2b[j]; // +1 si inside
        costemp4 = cosCache2b[j]; // +1 si inside


        glBegin (GL_QUAD_STRIP);
	    for (i = 0; i <= 64; i++)
	    {
		glNormal3f(sinCache2a[i] * sintemp3, cosCache2a[i] * sintemp3, costemp3);
		glTexCoord2f(1 - (float) i / 64, 1 - (float) (j+1) / 64); //si inside (j+1) -> j
		glVertex3f(sintemp2 * sinCache1a[i], sintemp2 * cosCache1a[i], zHigh); //si inside sintemp1 à la place de sintemp2 aux deux, zHigh -> zLow
    	    
		glNormal3f(sinCache2a[i] * sintemp4, cosCache2a[i] * sintemp4, costemp4);
		glTexCoord2f(1 - (float) i / 64, 1 - (float) j / 64); //si inside j -> j+1
		glVertex3f(sintemp1 * sinCache1a[i], sintemp1 * cosCache1a[i], zLow); //si inside sintemp1 -> sintemp2, zLow -> zHigh
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

