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
 * Detailed fish/fish2 and coral 3D models made by Biswajyoti Mahanta.
 *
 * Butterflyfish and Chromis 3D models/auto-generated code by "unpush"
 */

/*
 * Based on atlantis xscreensaver http://www.jwz.org/xscreensaver/
 */

/* atlantis --- Shows moving 3D sea animals */

/* Copyright (c) E. Lassauge, 1998. */

/*
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * The original code for this mode was written by Mark J. Kilgard
 * as a demo for openGL programming.
 *
 * Porting it to xlock  was possible by comparing the original Mesa's morph3d
 * demo with it's ported version to xlock, so thanks for Marcelo F. Vianna
 * (look at morph3d.c) for his indirect help.
 *
 * Thanks goes also to Brian Paul for making it possible and inexpensive
 * to use OpenGL at home.
 *
 * My e-mail address is lassauge@users.sourceforge.net
 *
 * Eric Lassauge  (May-13-1998)
 *
 */

/**
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */


/*
 * Coordinate system:
 * clockwise from top
 * with x=radius, y=0 the "top" (towards 1st desktop from above view)
 * and the z coordinate as height.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include "atlantis.h"

COMPIZ_PLUGIN_20090315 (atlantis, AtlantisPluginVTable);

int atlantisDisplayPrivateIndex;

int cubeDisplayPrivateIndex;


void
AtlantisScreen::initAtlantis ()
{
    unsigned int i = 0, i2 = 0;
    unsigned int j, k;
    unsigned int num;

    CompOption::Value::Vector cType   = optionGetCreatureType ();
    CompOption::Value::Vector cNumber = optionGetCreatureNumber ();
    CompOption::Value::Vector cSize   = optionGetCreatureSize ();
    CompOption::Value::Vector cColor  = optionGetCreatureColor ();

    num = MIN (cType.size (), cNumber.size ());
    num = MIN (num, cSize.size ());
    num = MIN (num, cColor.size ());

    mWater = NULL;
    mGround = NULL;

    mNumFish = 0;
    mNumCrabs = 0;

    for (k = 0; k < num; k++)
    {
	if (cSize.at (k).i () == 0)
	    continue;

	if (cType.at (k).i () != CRAB)
	    mNumFish += cNumber.at (k).i ();
	else
	    mNumCrabs += cNumber.at (k).i ();
    }

    mFish = (fishRec *) calloc (mNumFish,  sizeof(fishRec));
    mCrab = (crabRec *) calloc (mNumCrabs, sizeof(crabRec));

    if (optionGetShowWater ())
	mWaterHeight = optionGetWaterHeight () * 100000 - 50000;
    else
	mWaterHeight = 50000;

    mOldProgress = 0;

    for (k = 0; k < num; k++)
    {
	for (j = 0; j < (unsigned int ) cNumber.at (k).i (); j++)
	{
	    int size = cSize.at (k).i ();
	    int type = cType.at (k).i ();

	    if (size==0)
		break;

	    if (type != CRAB)
	    {
		fishRec * fish = &(mFish[i]);

		fish->type = type;

		if (type == WHALE)
		    size /= 2;
		if (type == DOLPHIN)
		    size *= 2;
		if (type == SHARK)
		    size *= 3;

		fish->size = randf (sqrtf (size) ) + size;
		fish->speed = randf (150) + 50;

		if (j == 0)
		    setSimilarColor4us (fish->color, cColor.at (k).c (),
		                        0.2, 0.1);
		else
		    setSimilarColor (fish->color, mFish[i-j].color,
		                     0.2, 0.1);

		fish->x = randf (size);
		fish->y = randf (size);
		fish->z = (mWaterHeight - 50000) / 2 +
			  randf (size * 0.02) - size * 0.01;
		fish->psi = randf (360) - 180.0;
		fish->theta = randf (100) - 50;
		fish->v = 1.0;

		fish->group = k;

		fish->boidsCounter = i % NUM_GROUPS;
		fish->boidsPsi = fish->psi;
		fish->boidsTheta = fish->theta;

		fish->smoothTurnCounter = NRAND (3);
		fish->smoothTurnAmount = NRAND (3) - 1;

		fish->prevRandPsi = 0;
		fish->prevRandTh = 0;

		i++;
	    }
	    else
	    {
		crabRec * crab = &(mCrab[i2]);

		crab->size = randf (sqrtf (size)) + size;
		crab->speed = randf (100) + 50;

		if (j == 0)
		    setSimilarColor4us (crab->color, cColor.at (k).c (),
		                        0.2, 0.1);
		else
		    setSimilarColor (crab->color, mCrab[i2 - j].color,
		                     0.2, 0.1);

		crab->x = randf (2 * size) - size;
		crab->y = randf (2 * size) - size;

		if (optionGetStartCrabsBottom ())
		{
		    crab->z = 50000;
		    crab->isFalling = false;
		}
		else
		{
		    crab->z = (mWaterHeight - 50000)/2;
		    crab->isFalling = true;
		}

		crab->psi = randf (360);
		crab->theta= 0;

		crab->scuttlePsi = 0;
		crab->scuttleAmount = NRAND (3) - 1;

		i2++;
	    }
	}
    }

    mNumCorals = 0;
    mNumAerators = 0;

    cType = optionGetPlantType ();
    cNumber = optionGetPlantNumber ();
    cSize = optionGetPlantSize ();
    cColor = optionGetPlantColor ();

    num = MIN (cType.size (), cNumber.size ());
    num = MIN (num, cSize.size ());
    num = MIN (num, cColor.size ());

    for (k = 0; k < num; k++)
    {
	switch (cType.at (k).i ()) {
	case 0:
	case 1:
	    mNumCorals += cNumber.at (k).i ();
	    break;

	case 2:
	    mNumAerators += cNumber.at (k).i ();
	    break;
	}
    }

    mCoral   = (coralRec *) calloc (mNumCorals,   sizeof(coralRec));
    mAerator = (aeratorRec *) calloc (mNumAerators, sizeof(aeratorRec));

    for (k = 0; k < (unsigned int ) mNumAerators; k++)
    {
	mAerator[k].numBubbles = 20;
	mAerator[k].bubbles = (Bubble *) calloc (mAerator[k].numBubbles,
		sizeof (Bubble));
    }

    initWorldVariables();

    updateWater (0); /* make sure normals are initialized */
    updateGround (0);

    loadModels();
}

void
AtlantisScreen::loadModels ()
{

    mCrabDisplayList = glGenLists (1);
    glNewList (mCrabDisplayList, GL_COMPILE);
    DrawCrab (0);
    glEndList ();

    mCoralDisplayList = glGenLists (1);
    glNewList (mCoralDisplayList, GL_COMPILE);
    optionGetLowPoly () ? DrawCoralLow (0) : DrawCoral (0);
    glEndList ();

    mCoral2DisplayList = glGenLists (1);
    glNewList (mCoral2DisplayList, GL_COMPILE);
    optionGetLowPoly () ? DrawCoral2Low (0) : DrawCoral2 (0);
    glEndList ();

    mBubbleDisplayList = glGenLists (1);
    glNewList (mBubbleDisplayList, GL_COMPILE);
    optionGetLowPoly () ? DrawBubble (0, 6) : DrawBubble (0, 9);
    glEndList ();
}

void
AtlantisScreen::freeModels ()
{
    glDeleteLists (mCrabDisplayList, 1);
    glDeleteLists (mCoralDisplayList, 1);
    glDeleteLists (mCoral2DisplayList, 1);
    glDeleteLists (mBubbleDisplayList, 1);
}

float
AtlantisScreen::calculateRatio ()
{
    float temp, ratio;
    unsigned int i;

    if (!optionGetRescaleWidth ())
	return 1.0f;

    ratio = (float) screen->width () / (float) screen->height ();

    if (screen->outputDevs ().size () <= 1)
	return ratio;

    temp = 0;

    if (cubeScreen->multioutputMode () == CubeScreen::Automatic &&
    	(unsigned int) cubeScreen->nOutput () <
	screen->outputDevs ().size ())
    {
	return ratio;
    }
    else if (cubeScreen->multioutputMode () == CubeScreen::OneBigCube)
    {
	/* this doesn't seem right, but it works */
	for (i = 0; i < screen->outputDevs ().size (); i++)
	    temp += (float) screen->width () /
	    		(float) screen->outputDevs (). at (0).height ();

	if (temp != 0)
	    ratio = temp / screen->outputDevs ().size ();
    }
    else
    {
	for (i = 0; i < screen->outputDevs ().size (); i++)
	    temp += (float) screen->outputDevs ().at (0).width () /
	    	    (float) screen->outputDevs ().at (0).height ();

	if (temp != 0)
	    ratio = temp / screen->outputDevs ().size ();
    }

    return ratio;
}

void
AtlantisScreen::initWorldVariables ()
{
    unsigned int i = 0, i2 = 0;
    unsigned int j, k;
    int bi;
    unsigned int num;

    coralRec * coral;
    aeratorRec * aerator;

    CompOption::Value::Vector cType = optionGetPlantType ();
    CompOption::Value::Vector cNumber = optionGetPlantNumber ();
    CompOption::Value::Vector cSize = optionGetPlantSize ();
    CompOption::Value::Vector cColor = optionGetPlantColor ();

    mSpeedFactor = optionGetSpeedFactor ();

    mHsize = screen->vpSize ().width () * cubeScreen->nOutput ();

    mArcAngle = 360.0f / mHsize;
    mRadius = (100000 - 1) * cubeScreen->distance () /
		 cosf (0.5 * (mArcAngle * toRadians));
    mTopDistance = (100000 - 1) * cubeScreen->distance ();

    mRatio = calculateRatio ();

    mSideDistance = mTopDistance * mRatio;
    /* the 100000 comes from scaling by 0.00001 ( = (1/0.00001) ) */

    num = MIN (cType.size (), cNumber.size ());
    num = MIN (num, cSize.size ());
    num = MIN (num, cColor.size ());

    for (k = 0; k < num; k++)
    {
	for (j = 0; j < (unsigned int ) cNumber.at (k).i (); j++)
	{
	    int size = cSize.at (k).i ();

	    switch (cType.at (k).i ()) {
	    case 0:
	    case 1:
		coral = &(mCoral[i]);

		coral->size = (randf (sqrtf(size)) + size);
		coral->type = cType.at (k).i ();

		if (j == 0)
		    setSimilarColor4us (coral->color, cColor.at (k).c (),
		                        0.2, 0.2);
		else
		    setSimilarColor (coral->color, mCoral[i - j].color,
		                     0.2, 0.2);

		coral->psi = randf (360);

		setRandomLocation (&(coral->x), &(coral->y), 3 * size);
		coral->z = -50000;
		i++;
		break;

	    case 2:
		aerator = &(mAerator[i2]);

		aerator->size = randf (sqrtf (size)) + size;
		aerator->type = cType.at (k).i ();

		if (j == 0)
		    setSimilarColor4us (aerator->color, cColor.at (k).c (),
		                        0, 0);
		else
		    setSimilarColor (aerator->color, mAerator[i2-j].color,
		                     0.0, 0.0);

		setRandomLocation (&(aerator->x), &(aerator->y), size);
		aerator->z = -50000;

		for (bi = 0; bi < aerator->numBubbles; bi++)
		{
		    aerator->bubbles[bi].size = size;
		    aerator->bubbles[bi].x = aerator->x;
		    aerator->bubbles[bi].y = aerator->y;
		    aerator->bubbles[bi].z = aerator->z;
		    aerator->bubbles[bi].speed = 100 + randf (150);
		    aerator->bubbles[bi].offset = randf (2 * PI);
		    aerator->bubbles[bi].counter = 0;
		}

		i2++;
		break;
	    }
	}
    }

}

void
AtlantisScreen::freeAtlantis ()
{
    if (mFish)
	free (mFish);
    if (mCrab)
	free (mCrab);
    if (mCoral)
	free (mCoral);

    if (mAerator)
    {
	for (int i = 0; i < mNumAerators; i++)
	{
	    if (mAerator[i].bubbles)
		free (mAerator[i].bubbles);
	}

	free (mAerator);
    }

    freeWater (mWater);
    freeWater (mGround);

    mFish = NULL;
    mCrab = NULL;
    mCoral= NULL;
    mAerator = NULL;

    freeModels ();
}

void
AtlantisScreen::updateAtlantis ()
{
    freeAtlantis ();
    initAtlantis ();
}

void
AtlantisScreen::screenOptionChange (CompOption              *opt,
				    AtlantisScreen::Options num)
{
     updateAtlantis ();
}

void
AtlantisScreen::speedFactorOptionChange (CompOption              *opt,
				         AtlantisScreen::Options num)
{
    mSpeedFactor = optionGetSpeedFactor ();
}

void
AtlantisScreen::lightingOptionChange (CompOption              *opt,
				      AtlantisScreen::Options num)
{
    initLightPosition ();
}

void
AtlantisScreen::lowPolyOptionChange (CompOption              *opt,
				     AtlantisScreen::Options num)
{
    freeModels ();
    loadModels ();
}

void
AtlantisScreen::cubeClearTargetOutput (float xRotate,
				       float vRotate)
{

    cubeScreen->cubeClearTargetOutput (xRotate, vRotate);

    glClear (GL_DEPTH_BUFFER_BIT);
}

void
AtlantisScreen::setLightPosition (GLenum light)
{
    float position[] = { 0.0, 0.0, 1.0, 0.0 };
    float angle = optionGetLightInclination() * toRadians;

    if (optionGetRotateLighting ())
	angle = 0;

    position[1] = sinf (angle);
    position[2] = cosf (angle);

    glLightfv (light, GL_POSITION, position);
}

void
AtlantisScreen::initLightPosition ()
{
    glPushMatrix ();
    glLoadIdentity ();
    // gScreen->setLightPosition (GL_LIGHT1); // XXX
    glPopMatrix();
}

void
AtlantisScreen::cubePaintInside (const GLScreenPaintAttrib &attrib,
				 const GLMatrix		   &transform,
				 CompOutput		   *output,
				 int			   size)
{
    int i, j;

    float scale, ratio;

    static const float mat_shininess[] = { 60.0 };
    static const float mat_specular[] = { 0.6, 0.6, 0.6, 1.0 };
    static const float mat_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    static const float mat_ambient[] = { 0.8, 0.8, 0.9, 1.0 };

    static const float lmodel_localviewer[] = { 0.0 };
    static const float lmodel_twoside[] = { 0.0 };
    static       float lmodel_ambient[] = { 0.4, 0.4, 0.4, 0.4 };

    GLScreenPaintAttrib sAttrib = attrib;
    GLMatrix mT = transform;

    int new_hsize = screen->vpSize ().width () * cubeScreen->nOutput ();

    int drawDeformation = (mOldProgress == 0.0f ? getCurrentDeformation() :
						     getDeformationMode ());

    if (optionGetShowWater())
	mWaterHeight = optionGetWaterHeight() * 100000 - 50000;
    else
	mWaterHeight = 50000;

    ratio = calculateRatio ();

    if (new_hsize < mHsize || fabsf (ratio - mRatio) > 0.0001)
	updateAtlantis ();
    else if (new_hsize > mHsize)
    { /* let fish swim in their expanded enclosure without fully resetting */
	initWorldVariables ();
    }

    if (optionGetShowWater () || optionGetShowWaterWire () ||
	optionGetShowGround ())
    {
	updateDeformation (drawDeformation);
	updateHeight (mWater, optionGetShowGround () ? mGround : NULL,
	              optionGetWaveRipple(), drawDeformation);
    }

    sAttrib.yRotate += cubeScreen->invert () * (360.0f / size) *
    						 (cubeScreen->xRotations () -
	          				  (screen->vp ().x () *
	          				   cubeScreen->nOutput ()));

    gScreen->glApplyTransform (sAttrib, output, &mT);

    glPushMatrix();

    glLoadMatrixf (mT.getMatrix ());

    //if (!optionGetRotateLighting ())
	//gScreen->setLightPosition (GL_LIGHT1); // XXX

    glTranslatef (cubeScreen->outputXOffset (), -cubeScreen->outputYOffset (),
    									  0.0f);

    glScalef (cubeScreen->outputXScale (), cubeScreen->outputYScale (), 1.0f);

    bool enabledCull = false;

    glPushAttrib (GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);

    glEnable (GL_BLEND);
    glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    for (i=0; i<4; i++)
	lmodel_ambient[i] = optionGetLightAmbient();

    glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);
    glLightModelfv (GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    if (glIsEnabled (GL_CULL_FACE))
    {
	enabledCull = true;
    }

    if (optionGetShowWater ())
    {
	int cull;

	glGetIntegerv (GL_CULL_FACE_MODE, &cull);
	glEnable (GL_CULL_FACE);

	glCullFace (~cull & (GL_FRONT | GL_BACK));
	setWaterMaterial (optionGetWaterColor ());
	drawWater (mWater, true, false, drawDeformation);
	glCullFace (cull);
    }

    if (optionGetShowGround ())
    {
	setGroundMaterial (optionGetGroundColor ());

	if (optionGetRenderWaves () && optionGetShowWater () &&
	    !optionGetWaveRipple ())
	    drawGround (mWater, mGround, drawDeformation);
	else
	    drawGround (NULL, mGround, drawDeformation);
    }

    glPushMatrix();

    glColor4usv (defaultColor);

    glMaterialfv (GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

    glEnable  (GL_NORMALIZE);
    glEnable  (GL_DEPTH_TEST);
    glEnable  (GL_COLOR_MATERIAL);
    glEnable  (GL_LIGHTING);
    glEnable  (GL_LIGHT1);
    glDisable (GL_LIGHT0);

    glShadeModel(GL_SMOOTH);

    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glScalef (0.00001f / mRatio, 0.00001f, 0.00001f / mRatio);

    for (i = 0; i < mNumCrabs; i++)
    {
	glPushMatrix ();

	CrabTransform (& (mCrab[i]));

	scale = mCrab[i].size;
	scale /= 6000.0f;
	glScalef (scale, scale, scale);
	glColor4fv (mCrab[i].color);

	initDrawCrab ();
	glCallList (mCrabDisplayList);
	finDrawCrab ();

	glPopMatrix ();
    }

    for (i = 0; i < mNumCorals; i++)
    {
	glPushMatrix ();

	glTranslatef (mCoral[i].y, mCoral[i].z, mCoral[i].x);
	glRotatef (-mCoral[i].psi, 0.0, 1.0, 0.0);

	scale = mCoral[i].size;
	scale /= 6000.0f;
	glScalef (scale, scale, scale);
	glColor4fv (mCoral[i].color);

	switch (mCoral[i].type) {
	case 0:
	    initDrawCoral ();
	    glCallList (mCoralDisplayList);
	    finDrawCoral ();
	    break;
	case 1:
	    initDrawCoral2 ();
	    glCallList (mCoral2DisplayList);
	    finDrawCoral2 ();
	    break;
	}

	glPopMatrix ();
    }

    for (i = 0; i < mNumFish; i++)
    {
	glPushMatrix ();
	FishTransform (& (mFish[i]));
	scale = mFish[i].size;
	scale /= 6000.0f;
	glScalef (scale, scale, scale);
	glColor4fv (mFish[i].color);

	switch (mFish[i].type)
	{
	case SHARK:
	    DrawShark (& (mFish[i]), 0);
	    break;

	case WHALE:
	    DrawWhale (& (mFish[i]), 0);
	    break;

	case DOLPHIN:
	    DrawDolphin (& (mFish[i]), 0);
	    break;

	case BUTTERFLYFISH:
	    initDrawBFish (mFish[i].color);
	    AnimateBFish  (mFish[i].htail);
	    DrawAnimatedBFish ();
	    finDrawBFish ();
	    break;

	case CHROMIS:
	    initDrawChromis (mFish[i].color);
	    AnimateChromis (mFish[i].htail);
	    DrawAnimatedChromis ();
	    finDrawChromis ();
	    break;

	case CHROMIS2:
	    initDrawChromis2 (mFish[i].color);
	    AnimateChromis   (mFish[i].htail);
	    DrawAnimatedChromis ();
	    finDrawChromis ();
	    break;

	case CHROMIS3:
	    initDrawChromis3 (mFish[i].color);
	    AnimateChromis   (mFish[i].htail);
	    DrawAnimatedChromis ();
	    finDrawChromis ();
	    break;

	case FISH:
	    initDrawFish (mFish[i].color);
	    AnimateFish  (mFish[i].htail);
	    DrawAnimatedFish ();
	    finDrawFish ();
	    break;

	case FISH2:
	    initDrawFish2 (mFish[i].color);
	    AnimateFish2  (mFish[i].htail);
	    DrawAnimatedFish2 ();
	    finDrawFish2 ();
	    break;

	default:
	    break;
	}

	glPopMatrix();
    }

    glEnable(GL_CULL_FACE);

    for (i = 0; i < mNumAerators; i++)
    {
	for (j = 0; j < mAerator[i].numBubbles; j++)
	{
	    glPushMatrix ();

	    BubbleTransform (&(mAerator[i].bubbles[j]));
	    scale = mAerator[i].bubbles[j].size;

	    glScalef (scale, scale, scale);
	    glColor4fv (mAerator[i].color);

	    glCallList (mBubbleDisplayList);

	    glPopMatrix ();
	}
    }

    glPopMatrix ();

    if (optionGetShowWater () || optionGetShowWaterWire ())
    {
	glEnable (GL_CULL_FACE);
	setWaterMaterial (optionGetWaterColor ());
	drawWater (mWater, optionGetShowWater (),
		optionGetShowWaterWire (), drawDeformation);
    }


    if (optionGetShowGround ())
    {
	setGroundMaterial (optionGetGroundColor ());

	drawBottomGround (mGround, cubeScreen->distance (),
				      -0.5, drawDeformation);
    }
    else if (optionGetShowWater ())
    {
	setWaterMaterial (optionGetWaterColor ());
	drawBottomWater (mWater, cubeScreen->distance (),
				   -0.5, drawDeformation);
    }


    glDisable (GL_LIGHT1);
    glDisable (GL_NORMALIZE);

    if (!gScreen->lighting ())
	glDisable (GL_LIGHTING);

    glDisable (GL_DEPTH_TEST);

    if (enabledCull)
	glDisable (GL_CULL_FACE);

    glPopMatrix ();

    glPopAttrib ();

    mDamage = true;

    cubeScreen->cubePaintInside (sAttrib, transform, output, size);
}

void
AtlantisScreen::preparePaint (int ms)
{
    int i, j;

    bool currentDeformation = getCurrentDeformation ();
    int oldhsize = mHsize;

    updateWater ((float) ms / 1000.0f);
    updateGround ((float) ms / 1000.0f);

    /* temporary change for animals inside */
    if (currentDeformation == DeformationCylinder && mOldProgress > 0.9)
    {
	mHsize *= 32 / mHsize;
	mArcAngle = 360.0f / mHsize;
	mSideDistance = mRadius * mRatio;
    }
    else if (currentDeformation == DeformationSphere)
    {
	/* treat enclosure as a cylinder */
	mHsize *= 32 / mHsize;
	mArcAngle = 360.0f / mHsize;
	mSideDistance = mRadius * mRatio;

    }

    for (i = 0; i < mNumFish; i++)
    {
	FishPilot (i);

	/* animate fish tails */
	if (mFish[i].type <= FISH2)
	{
	    mFish[i].htail = fmodf (mFish[i].htail + 0.00025 *
	                               mFish[i].speed * mSpeedFactor, 1);
	}
    }

    for (i = 0; i < mNumCrabs; i++)
    {
	CrabPilot (i);
    }

    for (i = 0; i < mNumCorals; i++)
    {
	mCoral[i].z = getGroundHeight (mCoral[i].x, mCoral[i].y);
    }

    for (i = 0; i < mNumAerators; i++)
    {
	aeratorRec * aerator = &(mAerator[i]);
	float bottom = getGroundHeight (aerator->x, aerator->y);

	if (aerator->z < bottom)
	{
	    for (j = 0; j < aerator->numBubbles; j++)
	    {
		if (aerator->bubbles[j].counter == 0)
		    aerator->bubbles[j].z = bottom;
	    }
	}
	aerator->z = bottom;
	for (j = 0; j < aerator->numBubbles; j++)
	{
	    BubblePilot(i, j);
	}
    }

    mHsize = oldhsize;
    mArcAngle = 360.0f / mHsize;
    mSideDistance = mTopDistance * mRatio;

    cScreen->preparePaint (ms);
}

void
AtlantisScreen::donePaint ()
{
    if (mDamage)
    {
	cScreen->damageScreen ();
	mDamage = false;
    }

    cScreen->donePaint ();
}

/* One shot timer to ensure OpenGL is initialized before
 * we do anyting odd
 */

bool
AtlantisScreen::init ()
{
    initAtlantis ();

    optionSetSpeedFactorNotify  (boost::bind
    				 (&AtlantisScreen::speedFactorOptionChange, this, _1, _2));

    optionSetLowPolyNotify (boost::bind (&AtlantisScreen::lowPolyOptionChange, this, _1, _2));

    optionSetCreatureNumberNotify (boost::bind (&AtlantisScreen::screenOptionChange, this, _1, _2));
    optionSetCreatureSizeNotify   (boost::bind (&AtlantisScreen::screenOptionChange, this, _1, _2));
    optionSetCreatureColorNotify  (boost::bind (&AtlantisScreen::screenOptionChange, this, _1, _2));
    optionSetCreatureTypeNotify   (boost::bind (&AtlantisScreen::screenOptionChange, this, _1, _2));

    optionSetPlantNumberNotify (boost::bind (&AtlantisScreen::screenOptionChange, this, _1, _2));
    optionSetPlantSizeNotify   (boost::bind (&AtlantisScreen::screenOptionChange, this, _1, _2));
    optionSetPlantColorNotify  (boost::bind (&AtlantisScreen::screenOptionChange, this, _1, _2));
    optionSetPlantTypeNotify   (boost::bind (&AtlantisScreen::screenOptionChange, this, _1, _2));

    optionSetRescaleWidthNotify (boost::bind (&AtlantisScreen::screenOptionChange, this, _1, _2));

    optionSetRotateLightingNotify   (boost::bind (&AtlantisScreen::lightingOptionChange, this, _1, _2));
    optionSetLightInclinationNotify (boost::bind (&AtlantisScreen::lightingOptionChange, this, _1, _2));

    return false;
}

AtlantisScreen::AtlantisScreen (CompScreen *screen) :
    PluginClassHandler <AtlantisScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    cubeScreen (CubeScreen::get (screen)),
    mDamage (false),
    mFish (NULL),
    mCrab (NULL),
    mCoral (NULL),
    mAerator (NULL),
    mWater (NULL),
    mGround (NULL)
{
    CompTimer initTimer;
    static const float ambient[]  = { 0.0, 0.0, 0.0, 0.0 };
    static const float diffuse[]  = { 1.0, 1.0, 1.0, 1.0 };
    static const float specular[] = { 0.6, 0.6, 0.6, 1.0 };

    CompositeScreenInterface::setHandler (cScreen);
    CubeScreenInterface::setHandler (cubeScreen);

    glLightfv (GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv (GL_LIGHT1, GL_SPECULAR, specular);

    initLightPosition ();

    initTimer.setTimes (50, 50);
    initTimer.setCallback (boost::bind (&AtlantisScreen::init, this));
    initTimer.start ();
}

AtlantisScreen::~AtlantisScreen ()
{
    freeAtlantis ();
}

bool
AtlantisPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
    	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
    	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
    	!CompPlugin::checkPluginABI ("cube", COMPIZ_CUBE_ABI))
	return false;

    return true;
}
