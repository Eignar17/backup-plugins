/*
 * Animation plugin for compiz/beryl
 *
 * animation.c
 *
 * Copyright : (C) 2006 Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
 *
 * Particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                   : onestone@beryl-project.org
 *
 * Beam-Up added by : Florencio Guimaraes
 * E-mail           : florencio@nexcorp.com.br
 *
 * Hexagon tessellator added by : Mike Slegeir
 * E-mail                       : mikeslegeir@mail.utexas.edu>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "animationsim.h"

// =====================  Effect: RotateIn  =========================

void
RotateInAnim::applyTransform ()
{
    ANIMSIM_SCREEN (screen);

    GLMatrix *transform = &mTransform;
    float xRot, yRot;
    float angleX, angleY;
    float originX, originY;
    float forwardProgress;

    int direction = ass->optionGetRotateinDirection ();

    switch (direction)
    {
	case 1:
	    angleX = 0;
	    angleY = -ass->optionGetRotateinAngle ();
	    originX = WIN_X (mWindow);
	    originY = WIN_Y (mWindow) + WIN_H (mWindow);
	    break;
	case 2:
	    angleX = ass->optionGetRotateinAngle ();
	    angleY = 0;
	    originX = WIN_X (mWindow);
	    originY = WIN_Y (mWindow);
	    break;
	case 3:
	    angleX = 0;
	    angleY = ass->optionGetRotateinAngle ();
	    originX = WIN_X (mWindow);
	    originY = WIN_Y (mWindow);
	    break;
	case 4:
	    angleX = -ass->optionGetRotateinAngle ();
	    angleY = 0;
	    originX = WIN_X (mWindow) + WIN_W (mWindow);
	    originY = WIN_Y (mWindow);
	    break;
    }

    forwardProgress = getProgress ();
    xRot = (forwardProgress * angleX);
    yRot = (forwardProgress * angleY);

    transform->translate (WIN_X (mWindow) + WIN_W (mWindow) / 2.0f,
			  WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f,
			  0.0f);

    perspectiveDistortAndResetZ (*transform);

    transform->translate (-(WIN_X (mWindow) + WIN_W (mWindow) / 2.0f),
			  -(WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f),
			  0.0f);

    // animation movement
    transform->translate (originX, originY, 0.0f);

    transform->rotate (yRot, 1.0f, 0.0f, 0.0f);
    transform->rotate (xRot, 0.0f, 1.0f, 0.0f);

    transform->translate (-originX, -originY, 0.0f);

}

void 
RotateInAnim::prePaintWindow ()
{
    float forwardProgress = getProgress ();
    float xRot, yRot;
    float angleX, angleY;
    float originX, originY;
    Bool  xInvert = FALSE, yInvert = FALSE;
    int currentCull, invertCull;

    glGetIntegerv (GL_CULL_FACE_MODE, &currentCull);
    invertCull = (currentCull == GL_BACK) ? GL_FRONT : GL_BACK;

    ANIMSIM_SCREEN (screen);

    int direction = ass->optionGetRotateinDirection ();

    switch (direction)
    {
	case 1:
	    angleX = 0;
	    angleY = -ass->optionGetRotateinAngle ();
	    originX = WIN_X (mWindow);
	    originY = WIN_Y (mWindow) + WIN_H (mWindow);
	    break;
	case 2:
	    angleX = ass->optionGetRotateinAngle ();
	    angleY = 0;
	    originX = WIN_X (mWindow);
	    originY = WIN_Y (mWindow);
	    break;
	case 3:
	    angleX = 0;
	    angleY = ass->optionGetRotateinAngle ();
	    originX = WIN_X (mWindow);
	    originY = WIN_Y (mWindow);
	    break;
	case 4:
	    angleX = -ass->optionGetRotateinAngle ();
	    angleY = 0;
	    originX = WIN_X (mWindow) + WIN_W (mWindow);
	    originY = WIN_Y (mWindow);
	    break;
    }

    /* FIXME: This could be fancy vectorial normal direction calculation */

    xRot = fabs(fmodf(forwardProgress * angleX, 360.0f));
    yRot = fabs(fmodf(forwardProgress * angleY, 360.0f));

    if (xRot > 90.0f && xRot > 270.0f)
	xInvert = TRUE;

    if (yRot > 90.0f && yRot > 270.0f)
	yInvert = TRUE;

    if ((xInvert || yInvert) && !(xInvert && yInvert))
	glCullFace (invertCull);
}

void
RotateInAnim::postPaintWindow ()
{
    float forwardProgress = getProgress ();
    float xRot, yRot;
    float angleX, angleY;
    float originX, originY;
    Bool  xInvert = FALSE, yInvert = FALSE;
    int currentCull, invertCull;

    glGetIntegerv (GL_CULL_FACE_MODE, &currentCull);
    invertCull = (currentCull == GL_BACK) ? GL_FRONT : GL_BACK;

    ANIMSIM_SCREEN (screen);

    int direction = ass->optionGetRotateinDirection ();

    switch (direction)
    {
	case 1:
	    angleX = 0;
	    angleY = -ass->optionGetRotateinAngle ();
	    originX = WIN_X (mWindow);
	    originY = WIN_Y (mWindow) + WIN_H (mWindow);
	    break;
	case 2:
	    angleX = ass->optionGetRotateinAngle ();
	    angleY = 0;
	    originX = WIN_X (mWindow);
	    originY = WIN_Y (mWindow);
	    break;
	case 3:
	    angleX = 0;
	    angleY = ass->optionGetRotateinAngle ();
	    originX = WIN_X (mWindow);
	    originY = WIN_Y (mWindow);
	    break;
	case 4:
	    angleX = -ass->optionGetRotateinAngle ();
	    angleY = 0;
	    originX = WIN_X (mWindow) + WIN_W (mWindow);
	    originY = WIN_Y (mWindow);
	    break;
    }

    /* FIXME: This could be fancy vectorial normal direction calculation */

    xRot = fabs(fmodf(forwardProgress * angleX, 360.0f));
    yRot = fabs(fmodf(forwardProgress * angleY, 360.0f));

    if (xRot > 90.0f && xRot > 270.0f)
	xInvert = TRUE;

    if (yRot > 90.0f && yRot > 270.0f)
	yInvert = TRUE;

    /* We have to assume that invertCull will be
     * the actual inversion of our previous cull
     */

    if ((xInvert || yInvert) && !(xInvert && yInvert))
	glCullFace (invertCull);
}
