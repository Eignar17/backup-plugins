/*
 *
 * Compiz mouse cursor trails plugin
 *
 * mousetrails.c
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
#include <cstring>

#include "mousetrails.h"

COMPIZ_PLUGIN_20090315 (mousetrails, MousetrailsPluginVTable)

void
ParticleSystem::initParticles (int numParticles)
{

	if (mParticles)
		free(mParticles);
	//mParticles    = calloc(numParticles, sizeof(Particle));
	mParticles    = (Particle *) calloc(TEXTURES_SIZE, sizeof(Particle));

	mNumParticles = numParticles;
	mNumDisplayedParticles = 0;
	mSlowdown     = 1;
	mActive       = FALSE;
	mLastx	     = 0;
	mLasty        = 0;
	mLastGen      = 0;
	mInitialAlpha = 1;
	mSizeFactor   = 10;
	mColorcounter = 0;
	mX = 0;
	mY = 0;
	mSkip = 1;
	mAtSkip = 0;
	mSpeed = 1;
	mUseMousepoll = 1;

	mLastTextureFilled = -1;
	mLastCursorIndex = 0;
	mLastCursorSerial = 0;

	// Initialize cache
	mVertices_cache      = NULL;
	mColors_cache        = NULL;
	mCoords_cache        = NULL;
	mVertex_cache_count  = 0;
	mColor_cache_count   = 0;
	mCoords_cache_count  = 0;

	Particle *part = mParticles;
	int i;
	for (i = 0; i < TEXTURES_SIZE; i++, part++){
		part->a = 0.0f;
		part->r = 0.0f;
		part->g = 0.0f;
		part->b = 0.0f;
	}


	for (i = 0; i < TEXTURES_SIZE; i++){
		mCursors[i].cursor_serial = 0;
		mCursors[i].xhot = 0;
		mCursors[i].yhot = 0;
		mCursors[i].width = 0;
		mCursors[i].height = 0;
	}

	for (i = 0; i < TEXTURES_SIZE; i++) glGenTextures(1, &mCursors[i].texture);

}


void
ParticleSystem::drawParticles ()
{
	if (mNumDisplayedParticles == 0) return;

	glEnable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnableClientState(GL_COLOR_ARRAY);
	glBlendFunc(GL_SRC_ALPHA, mBlendMode);
	glEnable(GL_TEXTURE_2D);


	// Check that the caches are correct size 

	if (mVertex_cache_count != 1)
	{
		mVertices_cache =
			(GLfloat *) realloc(mVertices_cache,
					4 * 3 * sizeof(GLfloat));
		mVertex_cache_count = 1;
	}

	if (mCoords_cache_count != 1)
	{
		mCoords_cache =
			(GLfloat *) realloc(mCoords_cache,
					4 * 2 * sizeof(GLfloat));
		mCoords_cache_count = 1;
	}

	if (mColor_cache_count  != 1)
	{
		mColors_cache =
			(GLfloat *) realloc(mColors_cache,
					4 * 4 * sizeof(GLfloat));
		mColor_cache_count = 1;
	}


	GLfloat *vertices = mVertices_cache;
	GLfloat *coords   = mCoords_cache;
	GLfloat *colors   = mColors_cache;

	int cornersSize = sizeof (GLfloat) * 8;
	int colorSize   = sizeof (GLfloat) * 4;

	GLfloat cornerCoords[8] = {0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0};

	int numActive = 0;

	Particle *part = mParticles;
	int i;
	for (i = 0; i < mNumParticles; i++, part++)
	{
		//if (part->life > 0.0f)
		if (part->fade > 0.001f)
		{
			numActive += 4;

			float w = part->width / 2;
			float h = part->height / 2;

			w += (w * part->w_mod) * (1- part->a);
			h += (h * part->h_mod) * (1- part->a);

			vertices[0] = part->x - w;
			vertices[1] = part->y - h;
			vertices[2] = part->z;

			vertices[3] = part->x - w;
			vertices[4] = part->y + h;
			vertices[5] = part->z;

			vertices[6] = part->x + w;
			vertices[7] = part->y + h;
			vertices[8] = part->z;

			vertices[9]  = part->x + w;
			vertices[10] = part->y - h;
			vertices[11] = part->z;

			//vertices += 12;

			memcpy (coords, cornerCoords, cornersSize);

			//coords += 8;

			colors[0] = part->r;
			colors[1] = part->g;
			colors[2] = part->b;
			colors[3] = part->a; 
			memcpy (colors + 4, colors, colorSize);
			memcpy (colors + 8, colors, colorSize);
			memcpy (colors + 12, colors, colorSize);

			//colors += 16;

			if (mCursors[part->cursorIndex].texture)
			{
				glPushMatrix();

				glBindTexture(GL_TEXTURE_2D, mCursors[part->cursorIndex].texture);

				glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), mCoords_cache);
				glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), mVertices_cache);
				glColorPointer(4, GL_FLOAT, 4 * sizeof(GLfloat), mColors_cache);

				// draw particles
				glDrawArrays(GL_QUADS, 0, numActive);

				glPopMatrix();
			}

		}

	}


	glDisableClientState(GL_COLOR_ARRAY);
	glColor4usv(defaultColor);
	GLScreen::get(screen)->setTexEnvMode (GL_REPLACE);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

}

void
ParticleSystem::updateParticles (float time)
{
	int i;
	Particle *part;
	float speed    = (time / 5000.0);

	mActive = FALSE;

	part = mParticles;

	int numDisplayedParticles = 0;

	for (i = 0; i < mNumParticles; i++, part++)
	{
		if (part->a > 0.001f)
		{
			// modify opacity
			part->a -= 50 * part->a * part->fade * speed;
			mActive  = TRUE;

			numDisplayedParticles++;
		}
	}
	mNumDisplayedParticles = numDisplayedParticles;
}

void
ParticleSystem::finiParticles ()
{
	free(mParticles);
	if (mVertices_cache)
		free(mVertices_cache);
	if (mColors_cache)
		free(mColors_cache);
	if (mCoords_cache)
		free(mCoords_cache);

	int i;
	for (i = 0; i < TEXTURES_SIZE; i++) glDeleteTextures(1, &mCursors[i].texture);

}


/*
Window       root_return;
Window       child_return;
int          rootX, rootY;
int          winX, winY;
unsigned int maskReturn;
Bool         status;*/

void
ParticleSystem::genNewParticles (int time)
{
	MOUSETRAILS_SCREEN (screen);


	mSkip = ss->optionGetSkip () ;
	mAtSkip = (mAtSkip + 1) % mSkip;
	if (mAtSkip != 0) return;

	Bool rColor     = ss->optionGetRandom ();
	//float life      = ss->optionGetLife ();
	int sizeFactor  = mSizeFactor;
	//float lifeNeg   = 1 - life;
	//float fadeExtra = 0.05f * (1.01 - life);
	int cursorIndex = 0;

	unsigned short *c = ss->optionGetColor ();

	float colr1 = (float)c[0] / 0xffff;
	float colg1 = (float)c[1] / 0xffff;
	float colb1 = (float)c[2] / 0xffff;
	//float cola  = (float)c[3] / 0xffff;

	float partw = mCursors[mLastCursorIndex].width;
	float parth = mCursors[mLastCursorIndex].height;

	float partoffw = mCursors[mLastCursorIndex].xhot;
	float partoffh = mCursors[mLastCursorIndex].yhot;

	int livex;
	int livey;

	// get x cursor position (rootX and rootY)
	/*if (!mUseMousepoll){
		status = XQueryPointer (s->display->display, s->root,
			    &root_return, &child_return,
			    &rootX, &rootY, &winX, &winY, &maskReturn);

		livex = rootX;
		livey = rootY;
	}
	else
	{*/
		livex = ss->mPosX;
		livey = ss->mPosY;
	//}

	int deltax = livex - mX;
	int deltay = livey - mY;
	int delta = deltax * deltax + deltay * deltay;
	mX = livex;
	mY = livey;
	//if (delta < threshold * threshold) return;
	if (delta < 1 ) return;

	int newx = (float)(mX + partw/2 - partoffw);
	int newy = (float)(mY + parth/2 - partoffh);


	Particle *part = mParticles;
	int i;

	// possibly crashy
	if (ss->optionGetNumParticles() != mNumParticles){
	  //initParticles(ss->optionGetNumParticles (), ss->ps);
		mNumParticles = ss->optionGetNumParticles();
		mLastGen = -1;
			/*ps->slowdown = ss->optionGetSlowdown ();
			mInitialAlpha = ss->optionGetAlpha () ;
			mSizeFactor = ss->optionGetSize () ;
			mThreshold = ss->optionGetThreshold () ;
			mColorrate = ss->optionGetColorrate () ;*/
		for (i = 0; i < mNumParticles; i++, part++){
			part->a = 0.0f;
			part->r = 0.0f;
			part->g = 0.0f;
			part->b = 0.0f;
		}
	}
	

	//if (delta > threshold * threshold) max_new = 1;

	int nextGen = (mLastGen + 1) % mNumParticles;

	for (i = 0; i < mNumParticles; i++, part++)
	{
		if (i == nextGen)
		{
			// record the particle number being generated
			mLastGen = i;

			cursorIndex = ss->cursorUpdate ();

			mSlowdown = ss->optionGetSlowdown ();
			mInitialAlpha = ss->optionGetAlpha () ;
			mSizeFactor = ss->optionGetSize () ;
			mThreshold = ss->optionGetThreshold () ;
			mColorrate = ss->optionGetColorrate () ;

			// set the cursor array index
			part->cursorIndex = mLastCursorIndex = cursorIndex;

			part->fade = mSlowdown / 10.0;

			// set size
			part->width = partw;
			part->height = parth;
			part->w_mod = part->h_mod = ((float)sizeFactor - 10.0) / 10.0;

			// set direction
			part->xd = newx - mLastx;
			part->yd = newy - mLasty;

			// set position
			part->x = newx;
			part->y = newy;
			mLastx = newx;
			mLasty = newy;
			part->z  = 0.0;


			if (rColor)
			{
				float value;
				// linear smoothing
				value = mColorcounter;
				// sinwave smoothing - nothing spectacular
				//value = (float)mColorrate - ((float)mColorrate / 2) * (1 + cos (3.14159 * (float)mColorcounter / (float)mColorrate));

				if (mColorcounter == 0)
				{
					mR2 = (float)(random() & 0xff) / 255;
					mG2 = (float)(random() & 0xff) / 255;
					mB2 = (float)(random() & 0xff) / 255;
				}
				mR = ((float)mColorrate - value)/(float)mColorrate * mR + value/(float)mColorrate * mR2;
				mG = ((float)mColorrate - value)/(float)mColorrate * mG + value/(float)mColorrate * mG2;
				mB = ((float)mColorrate - value)/(float)mColorrate * mB + value/(float)mColorrate * mB2;
				part->r = mR;
				part->g = mG;
				part->b = mB;
				mColorcounter = (mColorcounter + 1) % mColorrate;
			}
			else
			{
				part->r = colr1;
				part->g = colg1;
				part->b = colb1;
			}
			// set transparancy
			//part->a = mInitialAlpha;
			float thresholdfactor = 1.0;
			if (delta < mThreshold) thresholdfactor = (float)delta / (float)mThreshold;
			part->a = mInitialAlpha * thresholdfactor;

			mActive = TRUE;
		}
	}
}

// damageRegion() seems to cause blockiness or jitters on the background window when scrolling, like on a webpage

void
MousetrailsScreen::damageRegion ()
{
	int      i;
	Particle *p;
	float    w, h, x1, x2, y1, y2;

	if (mPs->mNumDisplayedParticles == 0) return;

	if (!mPs)
		return;

	x1 = screen->width ();
	x2 = 0;
	y1 = screen->height ();
	y2 = 0;

	p = mPs->mParticles;

	for (i = 0; i < mPs->mNumParticles; i++, p++)
	{
		if (p->a > 0.001f){
			w = p->width / 2;
			h = p->height / 2;

			w += (w * p->w_mod) * p->a;
			h += (h * p->h_mod) * p->a;

			x1 = MIN (x1, p->x - w);
			x2 = MAX (x2, p->x + w);
			y1 = MIN (y1, p->y - h);
			y2 = MAX (y2, p->y + h);
		}
	}

	CompRegion r (floor (x1), floor (y1), ceil (x2) - floor (x1),
				  	      ceil (y2) - floor (y1));

	cScreen->damageRegion (r);
}

int
MousetrailsScreen::cursorUpdate ()
{

	Display * dpy = screen->dpy ();
	XFixesCursorImage *ci = XFixesGetCursorImage(dpy);

	/* Hack to avoid changing to an invisible (bugged)cursor image.
	 * Example: The animated firefox cursors.
	 */
	if (ci->width <= 1 && ci->height <= 1)
	{
		XFree (ci);
		return mPs->mLastCursorIndex;
	}

	// only update cursor if necessary (instead of all the time)
	if (ci->cursor_serial == mPs->mLastCursorSerial)
	{
		XFree (ci);
		return mPs->mLastCursorIndex;
	}

	// see if cursor already exists in textures
	int i;
	for (i = 0; i < TEXTURES_SIZE; i++){
		if (ci->cursor_serial == mPs->mCursors[i].cursor_serial){
			mPs->mLastCursorSerial = ci->cursor_serial;
			XFree (ci);
			return i;
		}
	}

	// otherwise grab the new cursor into textures
	int fillTexture = (mPs->mLastTextureFilled + 1) % TEXTURES_SIZE;

	unsigned char *pixels = (unsigned char *) malloc(ci->width * ci->height * 4);

	for (i = 0; i < ci->width * ci->height; i++)
	{
		unsigned long pix = ci->pixels[i];
		pixels[i * 4] = pix & 0xff;
		pixels[(i * 4) + 1] = (pix >> 8) & 0xff;
		pixels[(i * 4) + 2] = (pix >> 16) & 0xff;
		pixels[(i * 4) + 3] = (pix >> 24) & 0xff;
	}

	glBindTexture(GL_TEXTURE_2D, mPs->mCursors[fillTexture].texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ci->width, ci->height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	// save data for this cursor
	mPs->mCursors[fillTexture].cursor_serial = mPs->mLastCursorSerial = ci->cursor_serial;
	mPs->mCursors[fillTexture].xhot = ci->xhot;
	mPs->mCursors[fillTexture].yhot = ci->yhot;
	mPs->mCursors[fillTexture].width = ci->width;
	mPs->mCursors[fillTexture].height = ci->height;
	mPs->mLastTextureFilled = fillTexture;

	XFree (ci);
	free (pixels);

	return fillTexture;
}


void
MousetrailsScreen::positionUpdate (const CompPoint &p)
{
    int x = p.x ();
    int y = p.y ();

    mPosX = x;
    mPosY = y;
}



void
MousetrailsScreen::preparePaint (int time)
{
	//Bool useMousepoll = optionGetMousepoll ();

	if (mActive && !mPollHandle.active ())
	{
		CompPoint p = mPollHandle.getCurrentPosition ();
		mPosX = p.x ();
		mPosY = p.y ();
		mPollHandle.start ();
	}

	if (mActive && !mPs)
	{
		mPs = (ParticleSystem *) calloc(1, sizeof(ParticleSystem));
		if (!mPs)
		{
			cScreen->preparePaint (time);;
			return;
		}
		mPs->initParticles(optionGetNumParticles ());

		mPs->mSlowdown = optionGetSlowdown ();
		mPs->mInitialAlpha = optionGetAlpha () ;
		mPs->mThreshold = optionGetThreshold () ;
		mPs->mColorrate = optionGetColorrate () ;
		mPs->mSizeFactor = optionGetSize () ;
		mPs->mSkip = optionGetSkip () ;
		mPs->mBlendMode = GL_ONE_MINUS_SRC_ALPHA;
		//mPs->useMousepoll = optionGetMousepoll ();

		cursorUpdate ();

	}


	if (mPs && mPs->mActive)
	{
		mPs->updateParticles (time);
		damageRegion ();
	}

	if (mPs && mActive)
		mPs->genNewParticles (time);

	cScreen->preparePaint (time);

}

void
MousetrailsScreen::donePaint ()
{

	if (mActive || (mPs && mPs->mActive))
		damageRegion ();

	if (!mActive && mPollHandle.active () && mPs->mUseMousepoll)
	{
		mPollHandle.stop ();
	}

	if (!mActive && mPs && !mPs->mActive)
	{
		mPs->finiParticles ();
		free (mPs);
		mPs = NULL;
	}

	cScreen->donePaint ();
}

bool
MousetrailsScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
				  const GLMatrix	    &transform,
				  const CompRegion	    &region,
				  CompOutput		    *output,
				  unsigned int		    mask)
{
	bool           status;
	GLMatrix       sTransform (transform);

	status = gScreen->glPaintOutput (attrib, transform, region, output, mask);	

	if (!mPs || !mPs->mActive)
		return status;

// this doesn't seem to work
//	if (mask & PAINT_SCREEN_TRANSFORMED_MASK) return FALSE;

	sTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

	glPushMatrix ();
	glLoadMatrixf (sTransform.getMatrix ());

	mPs->drawParticles ();

	glPopMatrix();

	glColor4usv (defaultColor);

	return status;
}

bool
MousetrailsScreen::terminate (CompAction         *action,
			      CompAction::State  state,
			      CompOption::Vector options)
{
    mActive = false;
    damageRegion ();

    return true;
}

bool
MousetrailsScreen::initiate (CompAction         *action,
			     CompAction::State  state,
			     CompOption::Vector &options)
{

    if (mActive)
	return terminate (action, state, options);

    mActive = true;

    return true;
}

MousetrailsScreen::MousetrailsScreen (CompScreen *screen) :
    PluginClassHandler <MousetrailsScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mPosX (0),
    mPosY (0),
    mActive (false),
    mPs (NULL)
{
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen);

    mPollHandle.setCallback (boost::bind (&MousetrailsScreen::positionUpdate, this, _1));

    optionSetInitiateInitiate (boost::bind (&MousetrailsScreen::initiate, this, _1, _2, _3));
    optionSetInitiateTerminate (boost::bind (&MousetrailsScreen::terminate, this, _1, _2, _3));
}

MousetrailsScreen::~MousetrailsScreen ()
{
    if (mPollHandle.active ())
	mPollHandle.stop ();

    if (mPs && mPs->mActive)
    {
	mPs->finiParticles ();
	free (mPs);
	cScreen->damageScreen ();
    }
}

bool
MousetrailsPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) ||
	!CompPlugin::checkPluginABI ("mousepoll", COMPIZ_MOUSEPOLL_ABI))
	return false;

    return true;
}
