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

// =====================  Effect: Expand  =========================

void
ExpandAnim::applyTransform ()
{
    GLMatrix *transform = &mTransform;
    float defaultXScale = 0.3f;
    float forwardProgress;
    float expandProgress;
    const float expandPhaseEnd = 0.5f;

    forwardProgress = getProgress ();

    if ((1 - forwardProgress) < expandPhaseEnd)
	expandProgress = (1 - forwardProgress) / expandPhaseEnd;
    else
	expandProgress = 1.0f;

    // animation movement
    transform->translate (WIN_X (mWindow) + WIN_W (mWindow) / 2.0f,
			  WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f,
			  0.0f); 

    transform->scale (defaultXScale + (1.0f - defaultXScale) *
		      expandProgress,
		      (1 - forwardProgress), 0.0f);

    transform->translate (-(WIN_X (mWindow) + WIN_W (mWindow) / 2.0f),
		      	  -(WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f),
		      	  0.0f);

}

