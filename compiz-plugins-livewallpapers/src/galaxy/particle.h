/**
 *
 * Compiz galaxy livewallpaper plugin
 *
 * particle.h
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

#ifndef _PARTICLE_H
#define	_PARTICLE_H

COMPIZ_BEGIN_DECLS

#define _PI    (float) 3.141593
#define _2PI   (float) (2 * _PI)

#define ELLIPSE_RATIO .885f /* 0 (line) -> 1.0f (circle) */

/* Fovy angle a.k.a theta */
#define FOVY 45.0f
#define NEAR  0.1f
#define FAR  20.0f

typedef struct _Particle {
    GLfloat x, y, z;
    GLfloat angle;

    GLfloat distance;
    GLfloat size;
    GLfloat speed;

    GLubyte r, g, b, a;
} Particle;

typedef struct _ParticleSystem {
    /**
     * maxusr is promoted for a basis for calculating the other plugin's
     * parameters such pointsize, quadration etc.
     */
    GLfloat maxusr; /* You should not know why it's named to this.;) */

    GLint   particlesCount;
    GLint   particlesSize;

    Particle *particles;
    GLfloat  *vertices;
    GLubyte  *colors;

    GLint   rotateX;
    GLint   rotateY;
    GLint   rotateZ;
    GLfloat zoom;

    GLfloat offsetX;
    GLfloat offsetY;

    GLfloat speedRatio;

    GLuint  lightTexture;
    GLuint  flareTexture;

    GLboolean draw_streaks;

    GLfloat fovy; /* theta */
    GLfloat nearClip;
    GLfloat farClip;
    
    GLint   screenWidth;
    GLint   screenHeight;

} ParticleSystem;

COMPIZ_BEGIN_DECLS

#endif	/* _PARTICLE_H */
