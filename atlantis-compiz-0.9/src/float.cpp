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
 * Bubble movement code by David Mikos.
 */

#include "atlantis.h"
#include <math.h>
#include <float.h>

void
BubbleTransform(Bubble * bubble)
{
    glTranslatef (bubble->y, bubble->z, bubble->x);
}

void
AtlantisScreen::BubblePilot (int aeratorIndex,
            		     int bubbleIndex)
{
    int i;

    Bubble * bubble = &(mAerator[aeratorIndex].bubbles[bubbleIndex]);

    float x = bubble->x;
    float y = bubble->y;
    float z = bubble->z;

    float top = (optionGetRenderWaves () ? 100000*
	    	 getHeight(mWater, x / (100000 * mRatio),
	    	           y / (100000 * mRatio)) : mWaterHeight);

    float perpDist = (mSideDistance - bubble->size);
    float tempAng;
    float dist;


    z += mSpeedFactor * bubble->speed;

    if (z > top - 2 * bubble->size)
    {
	x = mAerator[aeratorIndex].x;
	y = mAerator[aeratorIndex].y;
	z = mAerator[aeratorIndex].z;
	bubble->speed   = 100 + randf (150);
	bubble->offset  = randf (2 * PI);
	bubble->counter = 0;
    }
    bubble->counter++;

    tempAng = fmodf (0.1 * bubble->counter * mSpeedFactor + bubble->offset,
                     2 * PI);
    x += 50 * sinf (tempAng);
    y += 50 * cosf (tempAng);

    tempAng = atan2f (y, x);
    dist    = hypotf (x, y);

    for (i = 0; i < mHsize; i++)
    {
	float directDist;
	float cosAng = cosf (fmodf (i * mArcAngle * toRadians - tempAng,
	                            2 * PI));
	if (cosAng <= 0)
	    continue;

	directDist = perpDist / cosAng;

	if (dist > directDist)
	{
	    x = directDist * cosf (tempAng);
	    y = directDist * sinf (tempAng);
	    tempAng = atan2f (y, x);
	    dist    = hypotf (x, y);
	}
    }

    bubble->x = x;
    bubble->y = y;
    bubble->z = z;
}
