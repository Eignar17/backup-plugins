/*
 * Compiz cube snowglobe plugin
 *
 * snowglobe.h
 *
 * This is a test plugin to show falling snow inside
 * of the transparent cube
 *
 * Written in 2007: Copyright (c) 2007 David Mikos
 * Ported to Compiz 0.9.x:
 * Copyright (c) 2010 Sam Spilsbury 
 * Compright (c) 2010 Scott Moreau
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

#ifndef _SNOWGLOBE_INTERNAL_H
#define _SNOWGLOBE_INTERNAL_H

#define LRAND()                 ((long) (random() & 0x7fffffff))
#define NRAND(n)                ((int) (LRAND() % (n)))
#define MAXRAND                 (2147483648.0) /* unsigned 1<<31 as a float */


#include <math.h>
#include <float.h>

/* some constants */
#define PI     M_PI
#define PIdiv2 M_PI_2
#define toDegrees (180.0f * M_1_PI)
#define toRadians (M_PI / 180.0f)

//return random number in range [0,x)
#define randf(x) ((float) (rand()/(((double)RAND_MAX + 1)/(x))))

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <cube/cube.h>

#include "snowglobe_options.h"

extern int snowglobeDisplayPrivateIndex;
extern int cubeDisplayPrivateIndex;


typedef struct _snowflakeRec
{
    float x, y, z;
    float theta, psi;
    float dpsi, dtheta;
    float speed, size;
}
snowflakeRec;

typedef struct _Vertex
{
    float v[3];
    float n[3];
}
Vertex;

typedef struct _Water
{
    int      size;
    float    distance;
    int      sDiv;

    float  bh;
    float  wa;
    float  swa;
    float  wf;
    float  swf;

    Vertex       *vertices;
    unsigned int *indices;

    unsigned int nVertices;
    unsigned int nIndices;

    unsigned int nSVer;
    unsigned int nSIdx;
    unsigned int nWVer;
    unsigned int nWIdx;

    float    wave1;
    float    wave2;
}
Water;

class SnowglobeScreen :
    public PluginClassHandler <SnowglobeScreen, CompScreen>,
    public CompositeScreenInterface,
    public CubeScreenInterface,
    public SnowglobeOptions
{
    public:
	
	SnowglobeScreen (CompScreen *);
	~SnowglobeScreen ();
	
	CompositeScreen *cScreen;
	GLScreen        *gScreen;
	CubeScreen      *csScreen;
	
	bool mDamage;
	
	int mNumSnowflakes;
	
	snowflakeRec *mSnow;
	
	Water *mWater;
	Water *mGround;
	
	float mXRotate;
	float mVRotate;
	
	float mWaterHeight; //water surface height
	
	int mHsize;
	float mDistance;    //perpendicular distance to wall from centre
	float mRadius;      //radius on which the hSize points lie
	float mArcAngle;    //360 degrees / horizontal size
	
	float mSpeedFactor; // multiply snowflake speeds by this value
	
	GLuint mSnowflakeDisplayList;
	
	void
	optionChange (CompOption	*option,
		      Options		num);
	
	void
	cubeClearTargetOutput (float	xRotate,
			       float	vRotate);
	
	void
	preparePaint (int ms);
	
	void
	cubePaintInside (const GLScreenPaintAttrib &sAttrib,
			 const GLMatrix            &transform,
			 CompOutput                *output,
			 int                       size);
	void
	donePaint ();
	
	void
	updateWater (float time);
	
	void
	updateGround (float time);
	
	void newSnowflakePosition (int);
	void SnowflakeDrift (int);
	
	void initializeWorldVariables ();
	
	void initSnowglobe ();
	void freeSnowglobe ();
	void updateSnowglobe ();
};

#define SNOWGLOBE_SCREEN(s)						      \
    SnowglobeScreen *as = SnowglobeScreen::get (s);

class SnowglobePluginVTable :
public CompPlugin::VTableForScreen <SnowglobeScreen>
{
public:
    
    bool init ();
};

//All calculations that matter with angles are done clockwise from top.
//I think of it as x=radius, y=0 being the top (towards 1st desktop from above view)
//and the z coordinate as height.

void
updateHeight (Water *w);

void
freeWater (Water *w);

void
drawWater (Water *w, bool full, bool wire);

void
drawGround (Water *w, Water *g);

void
drawBottomGround (int size, float distance, float bottom);

float
getHeight (Water *w, float x, float z);



void DrawSnowflake (int);
void initDrawSnowflake (void);
void finDrawSnowflake (void);

void DrawSnowman (int);

void SnowflakeTransform (snowflakeRec *);

#endif
