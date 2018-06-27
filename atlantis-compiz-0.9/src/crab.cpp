/*
 * Compiz cube atlantis plugin
 *
 * atlantis.c
 *
 * This plugin renders a fish tank inside of the transparent cube,
 * replete with fish, crabs, sand, bubbles, and coral.
 *
 * Copyright : (C) 2007-2008 by David Mikos
 * Email     : infiniteloopcounter@gmail.com
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
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

/*
 * This file is part of an automatically generated output from
 * executing objToC.java on a .obj 3D model. The output was
 * split manually into this code and a header with model data.
 */

#include "atlantis.h"
#include "crab.h"

void
DrawCrab(int wire)
{
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer (3, GL_FLOAT, 0, CrabPoints);
    glNormalPointer (GL_FLOAT, 0, CrabNormals);

    //Sphere
    GLenum cap = wire ? GL_LINE_LOOP : GL_TRIANGLES;
    glDrawElements(cap, 402, GL_UNSIGNED_INT, &(CrabIndices[0]));
    glDisableClientState (GL_NORMAL_ARRAY);
}

void
initDrawCrab()
{
    glRotatef(180.0, 0.0, 1.0, 0.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glDisable(GL_CULL_FACE);
}

void
finDrawCrab()
{
    glDisable(GL_CULL_FACE);
}
