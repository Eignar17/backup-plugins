/**
 *
 * Compiz floatingobjects plugin
 *
 * floatingobjects.c
 *
 * Copyright (c) 2006 Eckhart P. <beryl@cornergraf.net>
 * Copyright (c) 2006 Brian Jørgensen <qte@fundanemt.com>
 * Maintained by Danny Baumann <maniac@opencompositing.org>
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
 **/

/*
 * Many thanks to Atie H. <atie.at.matrix@gmail.com> for providing
 * a clean plugin template
 * Also thanks to the folks from #beryl-dev, especially Quinn_Storm
 * for helping me make this possible
 */
 
 /* This plugin is split up into three files. The first is much like the regular
   * snow plugin, but without the movement code, the second is a parser for
   * movement profiles (I'll have to write some documentation and/or find 
   * a parser) and the third is a set of options that moves an object based on
   * what the parser finds */
   
#include "fobjects-internal.h"

static void
fobjectsThink (FObjectsScreen *ss,
	   FObjectsObject  *sf)
{
    int boxing;

    boxing = fobjectsGetScreenBoxing (ss->s->display);

    if (sf->y >= ss->s->height + boxing ||
	sf->x <= -boxing ||
	sf->y >= ss->s->width + boxing ||
	sf->z <= -((float) fobjectsGetScreenDepth (ss->s->display) / 500.0) ||
	sf->z >= 1)
    {
	initiateFObjectsObject (ss, sf);
    }
    fobjectsMove (ss->s->display, sf);
}

static void
fobjectsMove (CompDisplay *d,
	  FObjectsObject   *sf)
{
    float tmp = 1.0f / (101.0f - fobjectsGetObjectSpeed (d));
    int   fobjectsUpdateDelay = fobjectsGetObjectUpdateDelay (d);
    
    sf->x += (sf->xs * (float) fobjectsUpdateDelay) * tmp;
    sf->y += (sf->ys * (float) fobjectsUpdateDelay) * tmp;
    sf->z += (sf->zs * (float) fobjectsUpdateDelay) * tmp;
    sf->ra += ((float) fobjectsUpdateDelay) / (10.0f - sf->rs);
}

static Bool
stepObjectPositions (void *closure)
{
    CompScreen *s = closure;
    int        i, numObjects;
    FObjectsObject  *fobjectsObject;
    Bool       onTop;

    SNOW_SCREEN (s);

    if (!ss->active)
	return TRUE;

    fobjectsObject = ss->allObjects;
    numObjects = fobjectsGetNumObjects (s->display);
    onTop = fobjectsGetObjectOverWindows (s->display);

    for (i = 0; i < numObjects; i++)
	fobjectsThink(ss, fobjectsObject++);

    if (ss->active && !onTop)
    {
	CompWindow *w;

	for (w = s->windows; w; w = w->next)
	{
	    if (w->type & CompWindowTypeDesktopMask)
		addWindowDamage (w);
	}
    }
    else if (ss->active)
	damageScreen (s);

    return TRUE;
}

static Bool
fobjectsToggle (CompDisplay     *d,
	    CompAction      *action,
	    CompActionState state,
 	    CompOption      *option,
	    int             nOption)
{
    CompScreen *s;
    Window     xid;

    xid = getIntOptionNamed (option, nOption, "root", 0);
    s = findScreenAtDisplay (d, xid);

    if (s)
    {
	SNOW_SCREEN (s);
	ss->active = !ss->active;
	if (!ss->active)
	    damageScreen (s);
    }

    return TRUE;
}

/* --------------------  HELPER FUNCTIONS ------------------------ */

static inline int
getRand (int min,
	 int max)
{
    return (rand() % (max - min + 1) + min);
}

static inline float
mmRand (int   min,
	int   max,
	float divisor)
{
    return ((float) getRand(min, max)) / divisor;
};

/* --------------------------- RENDERING ------------------------- */
static void
setupDisplayList (FObjectsScreen *ss)
{
    float fobjectsSize = fobjectsGetObjectSize (ss->s->display);

    ss->displayList = glGenLists (1);

    glNewList (ss->displayList, GL_COMPILE);
    glBegin (GL_QUADS);

    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (0, 0, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (0, fobjectsSize, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (fobjectsSize, fobjectsSize, -0.0);
    glColor4f (1.0, 1.0, 1.0, 1.0);
    glVertex3f (fobjectsSize, 0, -0.0);

    glEnd ();
    glEndList ();
}

static void
beginRendering (FObjectsScreen *ss,
		CompScreen *s)
{
    if (fobjectsGetUseBlending (s->display))
	glEnable (GL_BLEND);

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (ss->displayListNeedsUpdate)
    {
	setupDisplayList (ss);
	ss->displayListNeedsUpdate = FALSE;
    }

    glColor4f (1.0, 1.0, 1.0, 1.0);
    if (ss->fobjectsTexturesLoaded && fobjectsGetUseTextures (s->display))
    {
	int j;

	for (j = 0; j < ss->fobjectsTexturesLoaded; j++)
	{
	    FObjectsObject *fobjectsObject = ss->allObjects;
	    int       i, numObjects = fobjectsGetNumObjects (s->display);
	    Bool      fobjectsRotate = fobjectsGetObjectRotation (s->display);

	    enableTexture (ss->s, &ss->fobjectsTex[j].tex,
			   COMP_TEXTURE_FILTER_GOOD);

	    for (i = 0; i < numObjects; i++)
	    {
		if (fobjectsObject->tex == &ss->fobjectsTex[j])
		{
		    glTranslatef (fobjectsObject->x, fobjectsObject->y, fobjectsObject->z);
	    	    if (fobjectsRotate)
			glRotatef (fobjectsObject->ra, 0, 0, 1);
	    	    glCallList (ss->fobjectsTex[j].dList);
    		    if (fobjectsRotate)
			glRotatef (-fobjectsObject->ra, 0, 0, 1);
	    	    glTranslatef (-fobjectsObject->x, -fobjectsObject->y, -fobjectsObject->z);
		}
		fobjectsObject++;
	    }
	    disableTexture (ss->s, &ss->fobjectsTex[j].tex);
	}
    }
    else
    {
	FObjectsObject *fobjectsObject = ss->allObjects;
	int       i, numObjects = fobjectsGetNumObjects (s->display);

	for (i = 0; i < numObjects; i++)
	{
	    glTranslatef (fobjectsObject->x, fobjectsObject->y, fobjectsObject->z);
	    glRotatef (fobjectsObject->ra, 0, 0, 1);
	    glCallList (ss->displayList);
	    glRotatef (-fobjectsObject->ra, 0, 0, 1);
	    glTranslatef (-fobjectsObject->x, -fobjectsObject->y, -fobjectsObject->z);
	    fobjectsObject++;
	}
    }

    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    if (fobjectsGetUseBlending (s->display))
    {
	glDisable (GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
}

/* ------------------------  FUNCTIONS -------------------- */

static Bool
fobjectsPaintOutput (CompScreen              *s,
		 const ScreenPaintAttrib *sa,
		 const CompTransform	 *transform,
		 Region                  region,
		 CompOutput              *output, 
		 unsigned int            mask)
{
    Bool status;

    SNOW_SCREEN (s);

    if (ss->active && !fobjectsGetObjectOverWindows (s->display))
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP (ss, s, paintOutput);
    status = (*s->paintOutput) (s, sa, transform, region, output, mask);
    WRAP (ss, s, paintOutput, fobjectsPaintOutput);

    if (ss->active && fobjectsGetObjectOverWindows (s->display))
    {
	CompTransform sTransform = *transform;

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

	glPushMatrix ();
	glLoadMatrixf (sTransform.m);
	beginRendering (ss, s);
	glPopMatrix ();
    }

    return status;
}

static Bool
fobjectsDrawWindow (CompWindow           *w,
		const CompTransform  *transform,
		const FragmentAttrib *attrib,
		Region               region,
		unsigned int         mask)
{
    Bool status;

    SNOW_SCREEN (w->screen);

    /* First draw Window as usual */
    UNWRAP (ss, w->screen, drawWindow);
    status = (*w->screen->drawWindow) (w, transform, attrib, region, mask);
    WRAP (ss, w->screen, drawWindow, fobjectsDrawWindow);

    /* Check whether this is the Desktop Window */
    if (ss->active && (w->type & CompWindowTypeDesktopMask) && 
	!fobjectsGetObjectOverWindows (w->screen->display))
    {
	beginRendering (ss, w->screen);
    }

    return status;
}

static void
initiateFObjectsObject (FObjectsScreen *ss,
		   FObjectsObject  *sf)
{
    /* TODO: possibly place objects based on FOV, instead of a cube. */
    int boxing = fobjectsGetScreenBoxing (ss->s->display);

    switch (fobjectsGetObjectDirection (ss->s->display))
    {
    case ObjectDirectionTopToBottom:
	sf->x  = mmRand (-boxing, ss->s->width + boxing, 1);
	sf->xs = mmRand (-1, 1, 500);
	sf->y  = mmRand (-300, 0, 1);
	sf->ys = mmRand (1, 3, 1);
	break;
    case ObjectDirectionBottomToTop:
	sf->x  = mmRand (-boxing, ss->s->width + boxing, 1);
	sf->xs = mmRand (-1, 1, 500);
	sf->y  = mmRand (ss->s->height, ss->s->height + 300, 1);
	sf->ys = -mmRand (1, 3, 1);
	break;
    case ObjectDirectionRightToLeft:
	sf->x  = mmRand (ss->s->width, ss->s->width + 300, 1);
	sf->xs = -mmRand (1, 3, 1);
	sf->y  = mmRand (-boxing, ss->s->height + boxing, 1);
	sf->ys = mmRand (-1, 1, 500);
	break;
    case ObjectDirectionLeftToRight:
	sf->x  = mmRand (-300, 0, 1);
	sf->xs = mmRand (1, 3, 1);
	sf->y  = mmRand (-boxing, ss->s->height + boxing, 1);
	sf->ys = mmRand (-1, 1, 500);
	break;
    default:
	break;
    }

    sf->z  = mmRand (-fobjectsGetScreenDepth (ss->s->display), 0.1, 5000);
    sf->zs = mmRand (-1000, 1000, 500000);
    sf->ra = mmRand (-1000, 1000, 50);
    sf->rs = mmRand (-1000, 1000, 1000);
}

static void
setObjectTexture (FObjectsScreen *ss,
		     FObjectsObject  *sf)
{
    if (ss->fobjectsTexturesLoaded)
	sf->tex = &ss->fobjectsTex[rand () % ss->fobjectsTexturesLoaded];
}

static void
updateObjectTextures (CompScreen *s)
{
    int       i, count = 0;
    float     fobjectsSize = fobjectsGetObjectSize(s->display);
    int       numObjects = fobjectsGetNumObjects(s->display);
    FObjectsObject *fobjectsObject;

    SNOW_SCREEN (s);
    SNOW_DISPLAY (s->display);

    fobjectsObject = ss->allObjects;

    for (i = 0; i < ss->fobjectsTexturesLoaded; i++)
    {
	finiTexture (s, &ss->fobjectsTex[i].tex);
	glDeleteLists (ss->fobjectsTex[i].dList, 1);
    }

    if (ss->fobjectsTex)
	free (ss->fobjectsTex);
    ss->fobjectsTexturesLoaded = 0;

    ss->fobjectsTex = calloc (1, sizeof (FObjectsTexture) * sd->fobjectsTexNFiles);

    for (i = 0; i < sd->fobjectsTexNFiles; i++)
    {
	CompMatrix  *mat;
	FObjectsTexture *sTex;

	ss->fobjectsTex[count].loaded =
	    readImageToTexture (s, &ss->fobjectsTex[count].tex,
				sd->fobjectsTexFiles[i].s,
				&ss->fobjectsTex[count].width,
				&ss->fobjectsTex[count].height);
	if (!ss->fobjectsTex[count].loaded)
	{
	    compLogMessage ("fobjects", CompLogLevelWarn,
			    "Texture not found : %s", sd->fobjectsTexFiles[i].s);
	    continue;
	}
	compLogMessage ("fobjects", CompLogLevelInfo,
			"Loaded Texture %s", sd->fobjectsTexFiles[i].s);
	
	mat = &ss->fobjectsTex[count].tex.matrix;
	sTex = &ss->fobjectsTex[count];

	sTex->dList = glGenLists (1);
	glNewList (sTex->dList, GL_COMPILE);

	glBegin (GL_QUADS);

	glTexCoord2f (COMP_TEX_COORD_X (mat, 0), COMP_TEX_COORD_Y (mat, 0));
	glVertex2f (0, 0);
	glTexCoord2f (COMP_TEX_COORD_X (mat, 0),
		      COMP_TEX_COORD_Y (mat, sTex->height));
	glVertex2f (0, fobjectsSize * sTex->height / sTex->width);
	glTexCoord2f (COMP_TEX_COORD_X (mat, sTex->width),
		      COMP_TEX_COORD_Y (mat, sTex->height));
	glVertex2f (fobjectsSize, fobjectsSize * sTex->height / sTex->width);
	glTexCoord2f (COMP_TEX_COORD_X (mat, sTex->width),
		      COMP_TEX_COORD_Y (mat, 0));
	glVertex2f (fobjectsSize, 0);

	glEnd ();
	glEndList ();

	count++;
    }

    ss->fobjectsTexturesLoaded = count;
    if (count < sd->fobjectsTexNFiles)
	ss->fobjectsTex = realloc (ss->fobjectsTex, sizeof (FObjectsTexture) * count);

    for (i = 0; i < numObjects; i++)
	setObjectTexture (ss, fobjectsObject++);
}

static Bool
fobjectsInitScreen (CompPlugin *p,
		CompScreen *s)
{
    FObjectsScreen *ss;
    int        i, numObjects = fobjectsGetNumObjects (s->display);
    FObjectsObject  *fobjectsObject;

    SNOW_DISPLAY (s->display);

    ss = calloc (1, sizeof(FObjectsScreen));
    if (!ss)
	return FALSE;

    s->base.privates[sd->screenPrivateIndex].ptr = ss;

    ss->s = s;
    ss->fobjectsTexturesLoaded = 0;
    ss->fobjectsTex = NULL;
    ss->active = FALSE;
    ss->displayListNeedsUpdate = FALSE;

    ss->allObjects = fobjectsObject = malloc (numObjects * sizeof (FObjectsObject));
    if (!fobjectsObject)
    {
	free (ss);
	return FALSE;
    }

    for (i = 0; i < numObjects; i++)
    {
	initiateFObjectsObject (ss, fobjectsObject);
	setObjectTexture (ss, fobjectsObject);
	fobjectsObject++;
    }

    updateObjectTextures (s);
    setupDisplayList (ss);

    WRAP (ss, s, paintOutput, fobjectsPaintOutput);
    WRAP (ss, s, drawWindow, fobjectsDrawWindow);

    ss->timeoutHandle = compAddTimeout (fobjectsGetObjectUpdateDelay (s->display),
					(float)
					fobjectsGetObjectUpdateDelay (s->display) *
					1.2,
					stepObjectPositions, s);

    return TRUE;
}

static void
fobjectsFiniScreen (CompPlugin *p,
		CompScreen *s)
{
    int i;

    SNOW_SCREEN (s);

    if (ss->timeoutHandle)
	compRemoveTimeout (ss->timeoutHandle);

    for (i = 0; i < ss->fobjectsTexturesLoaded; i++)
    {
	finiTexture (s, &ss->fobjectsTex[i].tex);
	glDeleteLists (ss->fobjectsTex[i].dList, 1);
    }

    if (ss->fobjectsTex)
	free (ss->fobjectsTex);

    if (ss->allObjects)
	free (ss->allObjects);

    UNWRAP (ss, s, paintOutput);
    UNWRAP (ss, s, drawWindow);

    free (ss);
}

static void
fobjectsDisplayOptionChanged (CompDisplay        *d,
			  CompOption         *opt,
			  FobjectsDisplayOptions num)
{
    SNOW_DISPLAY (d);

    switch (num)
    {
    case FobjectsDisplayOptionObjectSize:
	{
	    CompScreen *s;

	    for (s = d->screens; s; s = s->next)
	    {
		SNOW_SCREEN (s);
		ss->displayListNeedsUpdate = TRUE;
		updateObjectTextures (s);
	    }
	}
	break;
    case FobjectsDisplayOptionObjectUpdateDelay:
	{
	    CompScreen *s;

	    for (s = d->screens; s; s = s->next)
	    {
		SNOW_SCREEN (s);
					
		if (ss->timeoutHandle)
		    compRemoveTimeout (ss->timeoutHandle);
		ss->timeoutHandle =
		    compAddTimeout (fobjectsGetObjectUpdateDelay (d),
				    (float) fobjectsGetObjectUpdateDelay (d) * 1.2,
				    stepObjectPositions, s);
	    }
	}
	break;
    case FobjectsDisplayOptionNumObjects:
	{
	    CompScreen *s;
	    int        i, numObjects;
	    FObjectsObject  *fobjectsObject;

	    numObjects = fobjectsGetNumObjects (d);
	    for (s = d->screens; s; s = s->next)
    	    {
		SNOW_SCREEN (s);
		ss->allObjects = realloc (ss->allObjects,
					     numObjects * sizeof (FObjectsObject));
		fobjectsObject = ss->allObjects;

		for (i = 0; i < numObjects; i++)
		{
		    initiateFObjectsObject (ss, fobjectsObject);
    		    setObjectTexture (ss, fobjectsObject);
	    	    fobjectsObject++;
		}
	    }
	}
	break;
    case FobjectsDisplayOptionObjectTextures:
	{
	    CompScreen *s;
	    CompOption *texOpt;

	    texOpt = fobjectsGetObjectTexturesOption (d);

	    sd->fobjectsTexFiles = texOpt->value.list.value;
    	    sd->fobjectsTexNFiles = texOpt->value.list.nValue;

	    for (s = d->screens; s; s = s->next)
		updateObjectTextures (s);
	}
	break;
    default:
	break;
    }
}

static Bool
fobjectsInitDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    CompOption  *texOpt;
    FObjectsDisplay *sd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    sd = malloc (sizeof (FObjectsDisplay));
    if (!sd)
	return FALSE;

    sd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (sd->screenPrivateIndex < 0)
    {
	free (sd);
	return FALSE;
    }
	
    fobjectsSetToggleKeyInitiate (d, fobjectsToggle);
    fobjectsSetNumObjectsNotify (d, fobjectsDisplayOptionChanged);
    fobjectsSetObjectSizeNotify (d, fobjectsDisplayOptionChanged);
    fobjectsSetObjectUpdateDelayNotify (d, fobjectsDisplayOptionChanged);
    fobjectsSetObjectTexturesNotify (d, fobjectsDisplayOptionChanged);

    texOpt = fobjectsGetObjectTexturesOption (d);
    sd->fobjectsTexFiles = texOpt->value.list.value;
    sd->fobjectsTexNFiles = texOpt->value.list.nValue;

    d->base.privates[displayPrivateIndex].ptr = sd;

    return TRUE;
}

static void
fobjectsFiniDisplay (CompPlugin  *p,
		 CompDisplay *d)
{
    SNOW_DISPLAY (d);

    freeScreenPrivateIndex (d, sd->screenPrivateIndex);
    free (sd);
}

static CompBool
fobjectsInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) fobjectsInitDisplay,
	(InitPluginObjectProc) fobjectsInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
fobjectsFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) fobjectsFiniDisplay,
	(FiniPluginObjectProc) fobjectsFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
fobjectsInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
fobjectsFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

CompPluginVTable fobjectsVTable = {
    "fobjects",
    0,
    fobjectsInit,
    fobjectsFini,
    fobjectsInitObject,
    fobjectsFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &fobjectsVTable;
}
