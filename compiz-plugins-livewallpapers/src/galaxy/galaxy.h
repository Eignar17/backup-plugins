/**
 *
 * Compiz galaxy livewallpaper plugin
 *
 * galaxy.h
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

#ifndef _GALAXY_H
#define	_GALAXY_H

COMPIZ_BEGIN_DECLS

extern GLvoid       generateFlare (ParticleSystem *ps, GLboolean drawStreaks);

extern GLboolean    buildParticles (ParticleSystem *ps);
extern GLboolean    updateParticles (ParticleSystem *ps);

extern GLvoid       drawParticles (ParticleSystem *ps);
extern GLvoid       drawGalaxy (ParticleSystem *ps);


typedef struct _GalaxyDisplay {
    int screenPrivateIndex;
} GalaxyDisplay;

typedef struct _GalaxyScreen
{
    Bool active;

    ParticleSystem *ps;

    /* This texture comes from Options */
    /* TODO: Get rid off these. */
    CompTexture  lightTexture;
    unsigned int lightWidth;
    unsigned int lightHeight;

    CompTimeoutHandle compTimeout;

    DrawWindowProc    drawWindow;

} GalaxyScreen;

#define GET_GALAXY_DISPLAY(d)                            \
    ((GalaxyDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define GALAXY_DISPLAY(d)                                \
    GalaxyDisplay *gd = GET_GALAXY_DISPLAY (d)

#define GET_GALAXY_SCREEN(s, gd)                         \
    ((GalaxyScreen *) (s)->base.privates[(gd)->screenPrivateIndex].ptr)

#define GALAXY_SCREEN(s)                                 \
    GalaxyScreen *gs = GET_GALAXY_SCREEN (s, GET_GALAXY_DISPLAY (s->display))

COMPIZ_END_DECLS

#endif	/* _GALAXY_H */