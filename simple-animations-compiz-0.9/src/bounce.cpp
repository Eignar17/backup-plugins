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

// =====================  Effect: Bounce  =========================

float
BounceAnim::getProgress ()
{
    return progressLinear ();
}

void
BounceAnim::applyTransform ()
{
    float scale = 1.0f - (targetScale * (currBounceProgress) + currentScale * (1.0f - currBounceProgress));
    float forwardProgress = getProgress ();;
    float forwardProgressInc = 1.0f / bounceCount;

    /* last bounce, enure we are going for 0.0 */
    currBounceProgress = (((1 - forwardProgress) - lastProgressMax) / forwardProgressInc);

    if (currBounceProgress > 1.0f)
    {
	currentScale = targetScale;
	targetScale = -targetScale + targetScale / 2.0f;
	lastProgressMax = 1.0f - forwardProgress;
	currBounceProgress = 0.0f;
	nBounce++;
    }

    GLMatrix *transform = &mTransform;

    transform->translate (WIN_X (mWindow) + WIN_W (mWindow) / 2.0f,
			  WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f, 0.0f);

    transform->scale (scale, scale, 1.0f);

    transform->translate (-(WIN_X (mWindow) + WIN_W (mWindow) / 2.0f),
			  -(WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f), 0.0f);

}
