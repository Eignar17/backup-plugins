/*
 *
 * Compiz mouse cursor trails plugin
 *
 * mousetrails.h
 *
 * Copyright : (C) 2008 by Erik Johnson
 * 
 * From code originally written by Dennis Kasprzyk
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

#include <cmath>

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <mousepoll/mousepoll.h>

#include "mousetrails_options.h"

// in theory the texture array size should be >= trail size
#define TEXTURES_SIZE 51

class Particle
{
    public:
	//float life;			// particle life
	float fade;			// fade speed
	float width;			// particle width
	float height;			// particle height
	float w_mod;			// particle size modification during life
	float h_mod;			// particle size modification during life
	float r;			// red value
	float g;			// green value
	float b;			// blue value
	float a;			// alpha value
	float x;			// X position
	float y;			// Y position
	float z;			// Z position
	float xd;
	float yd;
	int cursorIndex;		// array index of cursor serial number
};

class MouseCursor
{
    public:
	GLuint texture;
	unsigned long cursor_serial;
	unsigned short xhot;
	unsigned short yhot;
	unsigned short width;
	unsigned short height;
};

class ParticleSystem
{
    public:
	int      mNumParticles;
	Particle *mParticles;
	float    mSlowdown;
	float    mThreshold;
	Bool     mActive;
	int      mX, mY;
	GLuint   mBlendMode;

	int mLastx, mLasty;
	float mDelta;
	int mNumDisplayedParticles;
	int mLastGen;
	float mInitialAlpha;
	int mSizeFactor;
	Bool mUseMousepoll;

	// for random colors
	int mColorcounter;
	float mR;			// red value
	float mG;			// green value
	float mB;			// blue value
	float mR2;			// red value
	float mG2;			// green value
	float mB2;			// blue value
	int mColorrate;
	int mSkip;
	int mAtSkip;
	int mSpeed;

	// Cursor data
	MouseCursor mCursors[TEXTURES_SIZE];          // rolling array of cursors grabbed from x
	int mLastTextureFilled;                       // last slot used in textures
	unsigned long mLastCursorSerial;              // last cursor serial number grabbed from X
	int mLastCursorIndex;                         // ...and its associated array index

	// Moved from drawParticles to get rid of spurious malloc's
	GLfloat *mVertices_cache;
	int     mVertex_cache_count;
	GLfloat *mCoords_cache;
	int     mCoords_cache_count;
	GLfloat *mColors_cache;
	int     mColor_cache_count;

	void
	initParticles (int f_numParticles);

	void
	drawParticles ();

	void
	updateParticles (float time);

	void
	finiParticles ();

	void
	genNewParticles (int time);

};

class MousetrailsScreen :
    public PluginClassHandler <MousetrailsScreen, CompScreen>,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public MousetrailsOptions
{
    public:

	MousetrailsScreen (CompScreen *screen);
	~MousetrailsScreen ();

    public:

	CompositeScreen *cScreen;
	GLScreen	*gScreen;

	int mPosX;
	int mPosY;

	Bool mActive;

	ParticleSystem *mPs;

	MousePoller mPollHandle;

	void
	preparePaint (int);

	bool
	glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix		 &,
		       const CompRegion		 &,
		       CompOutput		 *,
		       unsigned int		    );

	void
	donePaint ();

	void
	damageRegion ();

	void
	positionUpdate (const CompPoint &p);

	bool
	terminate (CompAction         *action,
		   CompAction::State  state,
		   CompOption::Vector options);

	bool
	initiate (CompAction         *action,
		  CompAction::State  state,
		  CompOption::Vector &options);

	int
	cursorUpdate ();

};

#define MOUSETRAILS_SCREEN(s)						      \
    MousetrailsScreen *ss = MousetrailsScreen::get (s);

class MousetrailsPluginVTable :
    public CompPlugin::VTableForScreen <MousetrailsScreen>
{
    public:

	bool init ();
};
