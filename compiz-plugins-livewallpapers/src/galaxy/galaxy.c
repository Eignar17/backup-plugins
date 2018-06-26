/**
 *
 * Compiz galaxy livewallpaper plugin
 *
 * galaxy.c
 *
 * Copyright (c) 2010 Pal Dorogi <pal.dorogi@gmail.com>
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

#include <compiz-core.h>

#include "galaxy_options.h"

#include "particle.h"
#include "galaxy.h"

static int displayPrivateIndex = 0;

static Bool
updateGalaxy (void *closure)
{
    CompScreen *s = closure;

    GALAXY_SCREEN (s);

    if (!gs->active)
	return TRUE;

    if (gs->ps)
        updateParticles (gs->ps);

    CompWindow *w;

    for (w = s->windows; w; w = w->next)
    {
        if (w->type & CompWindowTypeDesktopMask)
            addWindowDamage (w);
    }
    return TRUE;
}

static GLboolean
initParticleSystem (CompScreen *s)
{
    GALAXY_SCREEN (s);

    gs->ps = (ParticleSystem *) calloc (1, sizeof (ParticleSystem));
    if (!gs->ps)
        return FALSE;

    ParticleSystem *ps = gs->ps;

    ps->particlesCount = galaxyGetParticlesCount (s->display);
    ps->particlesSize  = galaxyGetParticlesSize (s->display);

    ps->particles       = NULL;
    ps->vertices        = NULL;
    ps->colors          = NULL;

    ps->rotateX         = galaxyGetRotateX (s->display);
    ps->rotateY         = galaxyGetRotateY (s->display);
    ps->rotateZ         = galaxyGetRotateZ (s->display);
    ps->zoom            = galaxyGetZoom (s->display);


    ps->lightTexture    = 0;
    ps->flareTexture    = 0;
    ps->draw_streaks    = galaxyGetDrawStreaks (s->display);
    
    ps->fovy            = FOVY; /* THETA */
    ps->nearClip        = NEAR;
    ps->farClip         = FAR;

    ps->screenWidth    = s->width;
    ps->screenHeight   = s->height;



    /* FIXME: The loadLightTexture should need to be cleaned */
    if (loadLightTexture (s))
    {
        /* Pass the texture to the ParticleSystem */
        ps->lightTexture = gs->lightTexture.name;
    }
    else
        return FALSE;

    buildParticles (gs->ps);

    return TRUE;
}

/*
 * FIXME: Find a better solution for loading Light and pass to ParticleSystem.
 */
Bool
loadLightTexture (CompScreen *s)
{
    GALAXY_SCREEN (s);
 
    if (!readImageToTexture (s, &gs->lightTexture, "galaxy-light.jpg",
			     &gs->lightWidth, &gs->lightHeight))
    {
        finiTexture (s, &gs->lightTexture);
        initTexture (s, &gs->lightTexture);

        return FALSE;
    }

    return TRUE;
    
}

static Bool
galaxyDrawWindow (CompWindow           *w,
		  const CompTransform  *transform,
		  const FragmentAttrib *attrib,
		  Region               region,
		  unsigned int         mask)
{
    Bool status;

    GALAXY_SCREEN (w->screen);

    /* First draw Window as usual */
    UNWRAP (gs, w->screen, drawWindow);
    status = (*w->screen->drawWindow) (w, transform, attrib, region, mask);
    WRAP (gs, w->screen, drawWindow, galaxyDrawWindow);

    if (gs->active && (w->type & CompWindowTypeDesktopMask))
    {
        /* TODO: Try to find a solution to use a custom background */
        /* clearTargetOutput (w->screen->display, GL_COLOR_BUFFER_BIT); */
        drawGalaxy (gs->ps);
    }

    return status;
}

static void
galaxyDisplayOptionChanged (CompDisplay          *d,
                            CompOption           *opt,
		            GalaxyDisplayOptions num)
{
    CompScreen *s;

    s = d->screens;
    if (!s)
        return;

    GALAXY_SCREEN (s);

    switch (num)
    {
        case GalaxyDisplayOptionParticlesCount:
        {
            /* TODO: Add dynamic particles recreation functions. */
        }
        case GalaxyDisplayOptionToggleKey:
        {
            /* TODO: Handle or get rid of it. */
        }
        case GalaxyDisplayOptionNum:
        {
            /* TODO: Handle or get rid of it. */
        }
        break;
        case GalaxyDisplayOptionParticlesSize:
        {
            if (gs->ps)
                gs->ps->particlesSize = galaxyGetParticlesSize (d);
        }
        break;
        case GalaxyDisplayOptionUpdateDelay:
        {
            if (gs->compTimeout)
                compRemoveTimeout (gs->compTimeout);

            gs->compTimeout = compAddTimeout (galaxyGetUpdateDelay(d),
                                              (float) galaxyGetUpdateDelay(d) *
				              1.2,
				              updateGalaxy, s);
        }
        break;
        case GalaxyDisplayOptionDrawStreaks:
        {
            if (gs->ps)
            {
                /* FIXME: Get rid of this variable. */
                gs->ps->draw_streaks = galaxyGetDrawStreaks (d);
                generateFlare (gs->ps,  gs->ps->draw_streaks);
            }
        }
        break;
        case GalaxyDisplayOptionRotateX:
        {
            if (gs->ps)
                gs->ps->rotateX = galaxyGetRotateX (d);
        }
        break;
        case GalaxyDisplayOptionRotateY:
        {
            if (gs->ps)
                gs->ps->rotateY = galaxyGetRotateY (d);
        }
        break;
        case GalaxyDisplayOptionRotateZ:
        {
            if (gs->ps)
                gs->ps->rotateZ = galaxyGetRotateZ (d);
        }
        break;
        case GalaxyDisplayOptionZoom:
        {
            if (gs->ps)
                gs->ps->zoom = galaxyGetZoom (d);
        }
        break;
    }
}

static Bool
galaxyDisplayToggleChanged (CompDisplay     *d,
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
	GALAXY_SCREEN (s);

        gs->active = !gs->active;
	if (!gs->active)
	    damageScreen (s);

        return TRUE;
    }

    return FALSE;
}

static Bool
galaxyInitDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    GalaxyDisplay *gd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
	return FALSE;

    gd = (GalaxyDisplay *) malloc (sizeof (GalaxyDisplay));
    if (!gd)
	return FALSE;
    
    gd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (gd->screenPrivateIndex < 0) {
	free (gd);
	return FALSE;
    }

    galaxySetParticlesCountNotify (d, galaxyDisplayOptionChanged);
    galaxySetParticlesSizeNotify (d, galaxyDisplayOptionChanged);
    galaxySetUpdateDelayNotify (d, galaxyDisplayOptionChanged);
    galaxySetDrawStreaksNotify (d, galaxyDisplayOptionChanged);
    galaxySetRotateXNotify (d, galaxyDisplayOptionChanged);
    galaxySetRotateYNotify (d, galaxyDisplayOptionChanged);
    galaxySetRotateZNotify (d, galaxyDisplayOptionChanged);
    galaxySetZoomNotify (d, galaxyDisplayOptionChanged);

    galaxySetToggleKeyInitiate (d, galaxyDisplayToggleChanged);

    d->base.privates[displayPrivateIndex].ptr = gd;

    return TRUE;
}

static Bool
galaxyInitScreen (CompPlugin *p,
		  CompScreen *s)
{
    GALAXY_DISPLAY (s->display);

    GalaxyScreen *gs = (GalaxyScreen *) calloc (1, sizeof (GalaxyScreen));
    if (!gs)
        return FALSE;

    gs->active = FALSE;

    s->base.privates[gd->screenPrivateIndex].ptr = gs;

    if (!initParticleSystem (s))
        return FALSE;

    WRAP (gs, s, drawWindow, galaxyDrawWindow);

    gs->compTimeout = compAddTimeout (galaxyGetUpdateDelay (s->display),
                                     (float) galaxyGetUpdateDelay (s->display) *
				      1.2,
				      updateGalaxy, s);

    initTexture (s, &gs->lightTexture);

    return TRUE;
}

static void
galaxyFiniDisplay (CompPlugin  *p,
		   CompDisplay *d)
{
    GALAXY_DISPLAY (d);

    freeScreenPrivateIndex (d, gd->screenPrivateIndex);
    free (gd);
}

static void
galaxyFiniScreen (CompPlugin *p,
  		  CompScreen *s)
{
    GALAXY_SCREEN (s);

    if (!gs) return;

    if (gs->compTimeout)
	compRemoveTimeout (gs->compTimeout);

    if (gs->ps)
    {
        ParticleSystem *ps;

        ps = gs->ps;
        if (ps->particles)
            free (ps->particles);

        if (ps->vertices)
            free (ps->vertices);

        if (ps->colors)
            free (ps->colors);

        free (ps);
    }

    UNWRAP (gs, s, drawWindow);

    finiTexture (s, &gs->lightTexture);

    free (gs);
}

static Bool
galaxyInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    
    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
galaxyFini (CompPlugin *p)
{
    if (displayPrivateIndex >= 0)
        freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
galaxyInitObject (CompPlugin *p,
		CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) galaxyInitDisplay,
	(InitPluginObjectProc) galaxyInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
galaxyFiniObject (CompPlugin *p,
		CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) galaxyFiniDisplay,
	(FiniPluginObjectProc) galaxyFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable galaxyVTable = {
    "galaxy",
    0,
    galaxyInit,
    galaxyFini,
    galaxyInitObject,
    galaxyFiniObject,
    0,
    0
};

CompPluginVTable*
getCompPluginInfo (void)
{
    return &galaxyVTable;
}