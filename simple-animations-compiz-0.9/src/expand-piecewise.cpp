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
#define DELTA 0.0001f

// =====================  Effect: ExpandPW  =========================

void
ExpandPWAnim::applyTransform ()
{
    ANIMSIM_SCREEN (screen);

    GLMatrix *transform = &mTransform;

    float forwardProgress = 1.0f - getProgress ();

    float initialXScale = ass->optionGetExpandpwInitialHoriz () / (float) mWindow->width ();
    float initialYScale = ass->optionGetExpandpwInitialVert ()  / (float) mWindow->height ();

    // animation movement
    transform->translate (WIN_X (mWindow) + WIN_W (mWindow) / 2.0f,
			  WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f,
			  0.0f);

    float xScale;
    float yScale;
    float switchPointP;
    float switchPointN;
    float delay = ass->optionGetExpandpwDelay ();

    if (ass->optionGetExpandpwHorizFirst ())
    {
        switchPointP = mWindow->width () / (float) (mWindow->width () + mWindow->height ()) + mWindow->height () / (float) (mWindow->width () + mWindow->height ()) * delay;
        switchPointN = mWindow->width () / (float) (mWindow->width () + mWindow->height ()) - mWindow->width () / (float) (mWindow->width () + mWindow->height ()) * delay;
        if(switchPointP >= 1.0f) switchPointP = 1.0f - DELTA;
        if(switchPointN <= 0.0f) switchPointN = 0.0f + DELTA;
        xScale = initialXScale + (1.0f - initialXScale) * (forwardProgress < switchPointN ? 1.0f - (switchPointN - forwardProgress)/switchPointN : 1.0f);
        yScale = initialYScale + (1.0f - initialYScale) * (forwardProgress > switchPointP ? (forwardProgress - switchPointP)/(1.0f-switchPointP) : 0.0f);
    }
    else
    {
        switchPointP = mWindow->height () / (float) (mWindow->width () + mWindow->height ()) + mWindow->width () / (float) (mWindow->width () + mWindow->height ()) * delay;
        switchPointN = mWindow->height () / (float) (mWindow->width () + mWindow->height ()) - mWindow->height () / (float) (mWindow->width () + mWindow->height ()) * delay;
        if(switchPointP >= 1.0f) switchPointP = 1.0f - DELTA;
        if(switchPointN <= 0.0f) switchPointN = 0.0f + DELTA;
        xScale = initialXScale + (1.0f - initialXScale) * (forwardProgress > switchPointP ? (forwardProgress - switchPointP)/(1.0f-switchPointP) : 0.0f);
        yScale = initialYScale + (1.0f - initialYScale) * (forwardProgress < switchPointN ? 1.0f - (switchPointN - forwardProgress)/switchPointN : 1.0f);
    }

    transform->scale (xScale, yScale, 0.0f);

    transform->translate (-(WIN_X (mWindow) + WIN_W (mWindow) / 2.0f),
			  -(WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f),
			  0.0f);

}
