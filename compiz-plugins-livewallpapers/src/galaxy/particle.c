/**
 *
 * Compiz galaxy livewallpaper plugin
 *
 * particle.c
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

#include <math.h>

#include <compiz-core.h>

#include "particle.h"

/*
 * Set the color
 *
 * O     155 176 255  #9bb0ff  Blue
 * B     170 191 255  #aabfff  Blue-White
 * A     202 215 255  #cad7ff  White
 * F     248 247 255  #f8f7ff  White
 * G     255 244 234  #fff4ea  Yellow-White
 * K     255 210 161  #ffd2a1  Orange
 * M     255 204 111  #ffcc6f  Red
 * Reference: http://www.vendian.org/mncharity/dir3/starcolor/
 * Use my personal favorite mneomnic:
 * Only Bored Astronomers Find Gratification Knowing Mnemonics:)
 */
unsigned char star_colors[7][3] =
{
    {155, 176, 255},
    {170, 191, 255},
    {202, 215, 255},
    {248, 247, 255},
    {255, 244, 234},
    {255, 210, 161},
    {255, 204, 111}
};

enum _star_type {O, B, A, F, G, K, M} star_type;

#define RED 0
#define GREEN 1
#define BLUE 2

#define FLARE_SIZE 32

static inline float
randf (void)
{
        return  rand () / (GLfloat) RAND_MAX;
}

static inline float
rand1f (float num)
{
        return  num * randf ();
}

static inline float
rand2f (float min, float max)
{
    return min + (max - min) * randf ();
}

static inline float
clampf (float num, float min, float max)
{
   if(num > max)
   {
     num = max;
   }
   else if (num < min)
   {
     num = min;
   }

   return num;
}

/*
 * Gaussian random generator
 * Using the polar form of the Box-Muller transformation
 */
static float
randng (void)
{
    static float y1, y2;
    static int have_next = FALSE;

    if (have_next)
    {
        have_next = FALSE;
        return y2;
    }

    float x1, x2, w;
    do
    {
        x1 = 2.0 * randf () - 1.0;
        x2 = 2.0 * randf () - 1.0;
        w = x1 * x1 + x2 * x2;
    } while (w >= 1.0);

    w = sqrt(-2.0 * log (w) / w);
    y1 = x1 * w;
    y2 = x2 * w;
    have_next = TRUE;

    return y1;
}

GLvoid
generateFlare (ParticleSystem *ps, GLboolean drawStreaks)
{
    GLint w, h;
    GLint i, x, y;
    GLfloat dx, dy, r, R;
    GLfloat fa; /* flare alpha */

    w = FLARE_SIZE;
    h = FLARE_SIZE;
    R = (w + h) / 4;
    GLubyte *data;

    data = (GLubyte *) malloc (w * h * 4);
    if (!data)
	return;

    for (x = 0; x < w; ++x)
    {
        for (y = 0; y < h; ++y)
        {

            dx = (R - x) / R;
            dy = (R - y) / R;
            r =  sqrt(dx * dx + dy * dy);
            /* r = exp(-(r));
            r *= r; */
            r = 1 - r; /* Comment out when you uncomment the 2 lines above */

            /* Draw flare */
            fa = r * r;
            if (r > 1)
                fa = 1;

            /* Draw streaks */
            if (drawStreaks && ((dx == 0.f) || (dy == 0.f)))
            {
               fa += r * 0.5;
               if (fa > 1) fa = 1;
            }
            
            if ((x == 0) || (x == (FLARE_SIZE - 1)) ||
                (y == 0) || (y == (FLARE_SIZE - 1)))
            {
                fa = 0;
            }

            i = (x + y * w) * 4;
            data[i + 0] = 255; /* Red */
            data[i + 1] = 255; /* Ggreen */
            data[i + 2] = 255; /* Blue */
            data[i + 3] = (GLubyte) (255.0f * fa); /* Alpha */
        }
    }

    if (&ps->flareTexture)
    {
        glDeleteTextures (1, &ps->flareTexture);
        ps->flareTexture = 0;
    }

    glGenTextures (1, &ps->flareTexture);
    glBindTexture (GL_TEXTURE_2D, ps->flareTexture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, data);
    if (data)
        free (data);
}

GLvoid
initParticle (Particle *p)
{
    float gauss;
    float distance;

    p->angle = rand1f (_2PI);

    gauss = randng ();
    distance = fabs (gauss) * 0.5f;
    p->distance = distance;

    p->x = cosf (distance * _2PI);
    p->y = sinf (distance * _2PI);

    /* z is based on the mix of Normal and Standard Cauchy distributions */
    p->z = (1 - distance) * 0.25 * randng () /
           (_PI * (1 + (2 * distance * distance)));
    if (distance < 0.2)
    {
        p->z *= 0.8f;
    }

    /*
     *  Velocity is based on the Sun's angular velocity (360 degree / 250m yrs)
     *  and -+50m yrs (reduced to 300k and 200k to slow down) and the distance.
     */
    /* FIXME: Need a proper speed calculation */
    p->speed = rand2f (0.0012f, 0.0018f) / distance * 0.5f;

    /* Color is based on the Star colors. See above */
    if (distance < 0.15f)
    {
        star_type = B;
    }
    else if (distance < 0.25f)
    {
        star_type = A;
    }
    else if (distance < 0.35f)
    {
        star_type = F;
    }
    else
    {
        star_type = G;
    }


    p->r = star_colors[star_type][RED];
    p->g = star_colors[star_type][GREEN];
    p->b = star_colors[star_type][BLUE];

    p->a = (GLubyte) clampf (255 - (1 - distance) * 100, 155, 255);

}

GLboolean
buildParticles (ParticleSystem *ps)
{
    
    if (ps->particles)
	free (ps->particles);

    ps->particles = calloc (1, sizeof (Particle) * ps->particlesCount);
    if (!ps->particles)
        return FALSE;

    if (ps->vertices)
	free (ps->vertices);
    ps->vertices = calloc (1, ps->particlesCount * 3 * sizeof (GLfloat));
    if (!ps->vertices)
        return FALSE;

    if (ps->colors)
	free (ps->colors);
    ps->colors = calloc (1, ps->particlesCount * 4 * sizeof (GLubyte));
    if (!ps->colors)
        return FALSE;

    /* TRUE: Draw Streaks, FALSE: do not */
    generateFlare (ps, ps->draw_streaks);

    int i;
    for (i = 0; i < ps->particlesCount; i++)
    {
        initParticle (&ps->particles[i]);
    }

    return TRUE;
}

GLvoid
updateParticles (ParticleSystem *ps)
{
    static float x, y, xi, yi, zi;
    static int i;

    Particle *particles = ps->particles;
    GLfloat  *vertices  = ps->vertices;
    GLubyte  *colors    = ps->colors;

    for (i = 0; i < ps->particlesCount; i++)
    {
        particles[i].angle = particles[i].angle + 
                             particles[i].speed * ps->speedRatio;

        x = particles[i].distance * cosf (particles[i].angle);
        y = particles[i].distance * sinf (particles[i].angle) * -ELLIPSE_RATIO;

        xi = particles[i].x * y + particles[i].y * x;
        yi = particles[i].y * y - particles[i].x * x;
        
        zi = particles[i].z;

        colors[0] = particles[i].r;
        colors[1] = particles[i].g;
        colors[2] = particles[i].b;
        colors[3] = particles[i].a;
        colors += 4;

        vertices[0] = xi;
        vertices[1] = yi;
        vertices[2] = zi;
        vertices += 3;
    }
}

static GLvoid
drawLight (ParticleSystem *ps)
{
    glBindTexture (GL_TEXTURE_2D, ps->lightTexture);
    glBegin(GL_QUADS);              // Draw Texture Mapped Quad
        glTexCoord2d(0.0f, 0.0f);   // First Texture Coord
        glVertex2f(-1.0f, -1.0f);   // First Vertex
        glTexCoord2d(1.0f, 0.0f);   // Second Texture Coord
        glVertex2f(1.0f, -1.0f);    // Second Vertex
        glTexCoord2d(1.0f, 1.0f);   // Third Texture Coord
        glVertex2f(1.0f, 1.0f);     // Third Vertex
        glTexCoord2d(0.0f, 1.0f);   // Fourth Texture Coord
        glVertex2f(-1.0f, 1.0f);    // Fourth Vertex
    glEnd();
}

GLvoid
drawParticles (ParticleSystem *ps)
{
    /* FIXME: Find a proper qudratic attenuation  */
    static GLfloat quadratic[] =  { 0.4f, .0f, .1f };
    static GLfloat max_size = .0f;

    glBindTexture (GL_TEXTURE_2D, ps->flareTexture);

    glPointParameterfv (GL_POINT_DISTANCE_ATTENUATION, quadratic);
    glPointParameterf (GL_POINT_FADE_THRESHOLD_SIZE, 60.0f);

    glGetFloatv (GL_POINT_SIZE_MAX, &max_size);
    glPointParameterf (GL_POINT_SIZE_MIN, .1f);
    #ifdef POINT_SIZE_MAX_WORKS
    /* FIXME: This does not work with my Intel's DRI driver. */
    glPointParameterf(GL_POINT_SIZE_MAX, max_size);
    #endif

    glDepthMask(GL_FALSE);
    glPointSize (ps->particlesSize *
                 ps->maxusr); /* It's faster than call the get func */

    glTexEnvf (GL_POINT_SPRITE_ARB, GL_COORD_REPLACE, GL_TRUE);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable (GL_POINT_SPRITE);

    /* Draw the Array */
    glEnableClientState (GL_COLOR_ARRAY);
    glVertexPointer (3, GL_FLOAT, 3 * sizeof (GLfloat), ps->vertices);
    glColorPointer (4, GL_UNSIGNED_BYTE, 4 * sizeof (GLubyte), ps->colors);
    glDrawArrays (GL_POINTS, 0, ps->particlesCount);
    glDisableClientState (GL_COLOR_ARRAY);

    glDisable (GL_POINT_SPRITE);
}

GLvoid
drawGalaxy (ParticleSystem *ps)
{

    glEnable (GL_TEXTURE_2D);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE); /* Additive */

    /* Reset and save the coordinate system before touching. */
    glMatrixMode (GL_PROJECTION);
    glPushMatrix ();
    glLoadIdentity ();

    /* Set the viewport to be the entire window. */
    glViewport (0, 0, ps->screenWidth, ps->screenHeight);

    /**
     * Using glFrustum instead of gluPrespective.
     * http://www.opengl.org/wiki/GluPerspective_code
     */
    double ymax, xmax;
    ymax = ps->nearClip * tanf (ps->fovy * _PI / 360.0);
    xmax = ymax * ((double) ps->screenWidth / (double) ps->screenHeight);
    /* Set the correct Galaxy perspective. */
    glFrustum (-xmax, xmax, -ymax, ymax, ps->nearClip, ps->farClip);

    glMatrixMode (GL_MODELVIEW);
    glPushMatrix ();
        glLoadIdentity ();
        /*
         * FIXME: Find a solution for proper background/wallpaper drawing
         drawBackground (ps);
        */

        glTranslatef (ps->offsetX, ps->offsetY, -ps->zoom * ps->maxusr);

        glRotatef (ps->rotateX, 1.0f, 0.0f, 0.0f); /* Rotate on X axis */
        glRotatef (ps->rotateY, 0.0f, 1.0f, 0.0f); /* Rotate on Y axis */
        glRotatef (ps->rotateZ, 0.0f, 0.0f, 1.0f); /* Rotate on Z axis */

        /* Draw particles. */
        drawParticles (ps);
        /* Draw light. */
        drawLight (ps);
        
    /* Restore the coordinate system. */
    glMatrixMode (GL_PROJECTION);
    glPopMatrix ();
    /* Restore the original perspective. */
    glMatrixMode (GL_MODELVIEW);
    glPopMatrix ();

    /* Restore blendig and other stuffs */
    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable (GL_TEXTURE_2D);
    glDisable (GL_BLEND);
    glColor4usv (defaultColor);
}
