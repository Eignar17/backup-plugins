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
 * A set of utility functions by David Mikos.
 */

#include "atlantis.h"
#include <math.h>
#include <float.h>

int
AtlantisScreen::getCurrentDeformation ()
{
    CompPlugin *p = NULL;
    const char plugin[] = "cubeaddon";
    p = CompPlugin::find (plugin);    
    if (p)
    {
	CompOption::Vector &options =  p->vTable->getOptions ();
	bool cylinderManualOnly = false;
	bool unfoldDeformation = true;
	
	cylinderManualOnly = CompOption::getBoolOptionNamed (options, 								 "cylinder_manual_only",
							     0);
	
	unfoldDeformation = CompOption::getBoolOptionNamed (options,
							    "unfold_deformation",
							    0);

	if (screen->vpSize ().height () * 
	    cubeScreen->nOutput () > 2 && screen->desktopWindowCount () &&
	    (cubeScreen->rotationState () == CubeScreen::RotationManual ||
	    (cubeScreen->rotationState () == CubeScreen::RotationChange &&
	    !cylinderManualOnly)) &&
	    (!cubeScreen->unfolded () || unfoldDeformation))
	{
	    int deformation_type = CompOption::getIntOptionNamed (options,
	    							  "deformation",
	    							  0);
	    
	    return deformation_type;
	}
    }
    return DeformationNone; // ???
}

int
AtlantisScreen::getDeformationMode ()
{
    CompPlugin *p = NULL;
    const char plugin[] = "cubeaddon";
    p = CompPlugin::find (plugin);
    if (p)
    {
    	CompOption::Vector &options = p->vTable->getOptions ();
	int deformation_type = CompOption::getIntOptionNamed (options,
						  	      "deformation",
						  	      0);
	
	return deformation_type;
    }
    return DeformationNone;
}


float
symmDistr()
{ /* returns number in range [-1, 1] with bias towards 0, symmetric about 0. */
    float x = 2 * randf(1)-1;
    return x * (1 - cbrtf (1 - fabsf (x)));
}

void
setColor (float* color,
          float r, float g, float b, float a,
	  float randomOffset,
	  float randomness)
{
    float ro = randf (randomOffset) - randomOffset / 2 - randomness / 2;

    color[0] = r + ro + randf (randomness);
    color[1] = g + ro + randf (randomness);
    color[2] = b + ro + randf (randomness);
    color[3] = a;

    int i;
    for (i = 0; i < 4; i++)
    {
	if (color[i] < 0)
	    color[i] = 0;
	else if (color[i] > 1)
	    color[i] = 1;
    }
}

void
setSimilarColor (float* color,
                 float* color2,
                 float randomOffset,
                 float randomness)
{
    float ro = randf (randomOffset) - randomOffset / 2 - randomness / 2;

    int i;

    color[0] = color2[0] + ro + randf (randomness);
    color[1] = color2[1] + ro + randf (randomness);
    color[2] = color2[2] + ro + randf (randomness);
    color[3] = color2[3];

    for (i = 0; i < 4; i++)
    {
	if (color[i] < 0)
	    color[i] = 0;
	else if (color[i] > 1)
	    color[i] = 1;
    }
}

void
setSimilarColor4us (float* color,
                    unsigned short * color2,
                    float randomOffset,
                    float randomness)
{
    float color2f[4];

    convert4usTof(color2, color2f);

    return setSimilarColor (color, color2f, randomOffset, randomness);
}

void
AtlantisScreen::setRandomLocation (float * x,
				   float * y,
				   float size)
{
    int sector = NRAND (mHsize);
    float ang = randf (mArcAngle * toRadians) -
		mArcAngle * toRadians / 2;
    float r = mRatio*mRadius - size / 2;
    float d = randf (1);
    float factor = cosf (0.5 * (mArcAngle * toRadians)) /
    		   cosf (0.5 * (mArcAngle * toRadians) - fabsf (ang));

    ang += (0.5 + ((float) sector)) * mArcAngle * toRadians;
    ang = fmodf (ang, 2 * PI);

    d = (1 - d * d) * r * factor;

    *x = d * cosf (ang);
    *y = d * sinf (ang);
}

void
setMaterialAmbientDiffuse (float * c, float aFactor, float dFactor)
{
    float ambient[4];
    float diffuse[4];

    copyColor (ambient, c, aFactor);
    copyColor (diffuse, c, dFactor);

    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
}

void
setMaterialAmbientDiffuse4us (unsigned short * c,
                              float aFactor,
                              float dFactor)
{
    float cf[4];

    convert4usTof(c, cf);

    return setMaterialAmbientDiffuse(cf, aFactor, dFactor);
}

void
copyColor (float* c,
           float* c2,
           float factor)
{
   c[0] = factor * c2[0];
   c[1] = factor * c2[1];
   c[2] = factor * c2[2];
   c[3] = c2[3];
}

void
convert4usTof (unsigned short * us,
               float * f)
{
    static const unsigned short maxUnsignedShort = ~0;
    float maxUnsignedShortf = (float) maxUnsignedShort;

    int i;

    for (i = 0; i < 4; i++)
	f[i] = us[i] / maxUnsignedShortf;
}
