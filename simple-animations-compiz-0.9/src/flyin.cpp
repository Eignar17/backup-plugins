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

// =====================  Effect: Flyin  =========================

void
FlyInAnim::applyTransform ()
{
    GLMatrix *transform = &mTransform;
    float offsetX, offsetY;
    float xTrans, yTrans;
    float forwardProgress;

    ANIMSIM_SCREEN (screen);


    int direction = ass->optionGetFlyinDirection ();
    float distance = ass->optionGetFlyinDistance ();

    switch (direction)
    {
	case 0:
	    offsetX = 0;	 
	    offsetY = distance;
	    break;
	case 1:
	    offsetX = distance;
	    offsetY = 0;
	    break;
	case 2:
	    offsetX = 0;
	    offsetY = -distance;
	    break;
	case 3:
	    offsetX = -distance;
	    offsetY = 0;
	    break;
	case 4:
	    offsetX = ass->optionGetFlyinDirectionX ();
	    offsetY = ass->optionGetFlyinDirectionY ();
	    break;
    }

    forwardProgress = progressLinear ();
    xTrans = -(forwardProgress * offsetX);
    yTrans = -(forwardProgress * offsetY);
    Point3d translation = Point3d (xTrans, yTrans, 0);

    // animation movement
    transform->translate (translation.x (), translation.y (), translation.z ());

}
