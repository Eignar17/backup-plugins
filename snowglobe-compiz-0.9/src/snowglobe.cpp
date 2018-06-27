/*
 * Compiz cube snowglobe plugin
 *
 * snowglobe.c
 *
 * This is a test plugin to show falling snow inside
 * of the transparent cube
 *
 * Written in 2007 by David Mikos
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
 * Based on atlantis and snow plugins
 */

/**
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
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
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include "snowglobe.h"

COMPIZ_PLUGIN_20090315 (snowglobe, SnowglobePluginVTable);

void
SnowglobeScreen::initSnowglobe ()
{
    mWater = NULL;
    mGround = NULL;

    mNumSnowflakes = optionGetNumSnowflakes ();

    mSnow = (snowflakeRec *) calloc (mNumSnowflakes, sizeof (snowflakeRec));

    initializeWorldVariables ();

    int i;
    for (i = 0; i< mNumSnowflakes; i++)
    {
	mSnow[i].size = optionGetSnowflakeSize()
		+sqrt(randf(optionGetSnowflakeSize()));

	newSnowflakePosition (i);

	mSnow[i].psi = randf(2*PI);
	mSnow[i].theta= randf(PI);

	mSnow[i].dpsi = randf(5);
	mSnow[i].dtheta = randf(5);

	mSnow[i].speed = randf(0.4)+0.2;

    }

    mWaterHeight = 50000;

    mSnowflakeDisplayList = glGenLists(1);
    glNewList(mSnowflakeDisplayList, GL_COMPILE);
    DrawSnowflake(0);
    glEndList();
}

void
SnowglobeScreen::initializeWorldVariables ()
{    
    mSpeedFactor = optionGetSpeedFactor();

    mHsize = screen->vpSize ().width () * csScreen->nOutput ();

    mArcAngle = 360.0f / mHsize;
    mRadius = csScreen->distance () / sinf (0.5 * (PI - mArcAngle * toRadians));
    mDistance = csScreen->distance ();
}

void
SnowglobeScreen::freeSnowglobe ()
{
    if (mSnow)
	free (mSnow);

    freeWater (mWater);
    freeWater (mGround);
	
    glDeleteLists(mSnowflakeDisplayList,  1);
}

void
SnowglobeScreen::updateSnowglobe ()
{
    freeSnowglobe ();
    initSnowglobe ();
}

void
SnowglobeScreen::optionChange (CompOption *opt,
			       SnowglobeOptions::Options num)
{
    updateSnowglobe ();
    
    mSpeedFactor = optionGetSpeedFactor ();
}

void
SnowglobeScreen::cubeClearTargetOutput (float xRotate,
				    float vRotate)
{
    csScreen->cubeClearTargetOutput (xRotate, vRotate);
    
    glClear (GL_DEPTH_BUFFER_BIT);
}

void
SnowglobeScreen::cubePaintInside (const GLScreenPaintAttrib &attrib,
				  const GLMatrix	    &transform,
				  CompOutput		    *output,
				  int			    size)
{
    int i;

    mWaterHeight = 50000;

    if (mHsize != screen->vpSize ().width ())
	updateSnowglobe ();

    static const float mat_shininess[] = { 60.0 };
    static const float mat_specular[] = { 0.8, 0.8, 0.8, 1.0 };
    static const float mat_diffuse[] = { 0.46, 0.66, 0.795, 1.0 };
    static const float mat_ambient[] = { 0.1, 0.1, 0.3, 1.0 };
    static const float lmodel_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
    static const float lmodel_localviewer[] = { 0.0 };

    GLScreenPaintAttrib sAttrib (attrib);
    GLMatrix mT (transform);

    if (optionGetShowWater())
	updateHeight(mWater);

    sAttrib.yRotate += csScreen->invert () * (360.0f / size) *
		       (csScreen->xRotations () - (screen->vp ().x () * csScreen->nOutput ()));

    gScreen->glApplyTransform (sAttrib, output, &mT);

    glPushMatrix();
    glLoadMatrixf(mT.getMatrix ());

    glTranslatef(-csScreen->outputXOffset (), -csScreen->outputYOffset (), 0.0f);

    glScalef(csScreen->outputXScale (), csScreen->outputYScale (), 1.0f);

    bool enabledCull = false;

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);

    glEnable(GL_BLEND);

    if (glIsEnabled(GL_CULL_FACE))
    {
	enabledCull = true;
    }

    int cull;

    glGetIntegerv(GL_CULL_FACE_MODE, &cull);
    glEnable(GL_CULL_FACE);

    glCullFace(~cull & (GL_FRONT | GL_BACK));

    if (optionGetShowWater())
    {
	glColor4usv(optionGetWaterColor());
	drawWater(mWater, true, false);
    }
    glCullFace(cull);

    if (optionGetShowGround())
    {
	glColor4f(0.8, 0.8, 0.8, 1.0);
	drawGround(NULL, mGround);
    }

    glPushMatrix();

    glColor4usv(defaultColor);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);

    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    for (i = 0; i < mNumSnowflakes; i++)
    {
	glPushMatrix ();
	SnowflakeTransform(&(mSnow[i]));

	float scale = 0.01 * mSnow[i].size;
	glScalef(scale, scale, scale);

	initDrawSnowflake ();
	glCallList(mSnowflakeDisplayList);
	finDrawSnowflake ();
	glPopMatrix ();
    }

    if (optionGetShowSnowman())
    {
	glPushMatrix ();

	float bottom = -0.5;
	if (optionGetShowGround ())
	    bottom = getHeight (mGround, 0, 0);
	glTranslatef (0, bottom, 0);

	float scale = 0.4 * optionGetSnowmanSize () * (0.5 - bottom);
	glScalef (scale, scale, scale);

	glColor4f (1.0, 1.0, 1.0, 1.0);

	DrawSnowman (0);
	glPopMatrix ();
    }

    glPopMatrix ();

    if (optionGetShowWater ())
    {
	glEnable(GL_CULL_FACE);
	glColor4usv(optionGetWaterColor());
	drawWater(mWater, optionGetShowWater(), 0);
    }

    if (optionGetShowGround())
    {
	glColor4f(0.8, 0.8, 0.8, 1.0);
	drawBottomGround(screen->vpSize ().width () * csScreen->nOutput (), csScreen->distance (), -0.4999);
    }

    glDisable(GL_LIGHT1);
    glDisable(GL_NORMALIZE);

    if (!gScreen->lighting ())
	glDisable(GL_LIGHTING);

    glDisable(GL_DEPTH_TEST);

    if (enabledCull)
	glDisable(GL_CULL_FACE);

    glPopMatrix();

    glPopAttrib();

    mDamage = true;
    
    csScreen->cubePaintInside (sAttrib, transform, output, size);
}

void
SnowglobeScreen::preparePaint (int ms)
{
    int i;

    for (i = 0; i < mNumSnowflakes; i++)
    {
	SnowflakeDrift (i);
    }

    updateWater ((float) ms / 1000.0);
    updateGround ((float ) ms / 1000.0);
    
    cScreen->preparePaint (ms);
}

void
SnowglobeScreen::donePaint ()
{
    if (mDamage)
    {
	cScreen->damageScreen ();
	mDamage = false;
    }
    
    cScreen->donePaint ();
}

SnowglobeScreen::SnowglobeScreen (CompScreen *s) :
    PluginClassHandler <SnowglobeScreen, CompScreen> (s),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    csScreen (CubeScreen::get (screen)),
    mDamage (false)
{
    static const float ambient[] = { 0.3, 0.3, 0.3, 1.0 };
    static const float diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    static const float position[] = { 0.0, 1.0, 0.0, 0.0 };
    
    CompositeScreenInterface::setHandler (cScreen, true);
    CubeScreenInterface::setHandler (csScreen, true);
    
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, position);
    
    initSnowglobe ();
    
    optionSetSpeedFactorNotify (boost::bind (&SnowglobeScreen::optionChange,
					       this, _1, _2));
    
    optionSetNumSnowflakesNotify (boost::bind (&SnowglobeScreen::optionChange,
					       this, _1, _2));
    optionSetSnowflakeSizeNotify (boost::bind (&SnowglobeScreen::optionChange,
					       this, _1, _2));
}

SnowglobeScreen::~SnowglobeScreen ()
{
    freeSnowglobe ();
}

bool
SnowglobePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
	!CompPlugin::checkPluginABI ("cube", COMPIZ_CUBE_ABI))
	return false;
    
    return true;
    
}