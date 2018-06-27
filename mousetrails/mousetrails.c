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

#include <math.h>   
#include <string.h>

#include <compiz-core.h>
#include <compiz-mousepoll.h>

#include "mousetrails_options.h"

#define GET_MOUSETRAILS_DISPLAY(d)                                  \
	((mousetrailsDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define MOUSETRAILS_DISPLAY(d)                      \
	mousetrailsDisplay *sd = GET_MOUSETRAILS_DISPLAY (d)

#define GET_MOUSETRAILS_SCREEN(s, sd)                                   \
	((mousetrailsScreen *) (s)->base.privates[(sd)->screenPrivateIndex].ptr)

#define MOUSETRAILS_SCREEN(s)                                                      \
	mousetrailsScreen *ss = GET_MOUSETRAILS_SCREEN (s, GET_MOUSETRAILS_DISPLAY (s->display))

// in theory the texture array size should be >= trail size
#define TEXTURES_SIZE 51

int mousetrailsCursorUpdate (CompScreen *s);

typedef struct _Particle
{
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
} Particle;

typedef struct _MouseCursor
{
	GLuint texture;
	unsigned long cursor_serial;
	unsigned short xhot;
	unsigned short yhot;
	unsigned short width;
	unsigned short height;
} MouseCursor;

typedef struct _ParticleSystem
{
	int      numParticles;
	Particle *particles;
	float    slowdown;
	float    threshold;
	Bool     active;
	int      x, y;
	GLuint   blendMode;

	int lastx, lasty;
	float delta;
	int numDisplayedParticles;
	int lastGen;
	float initialAlpha;
	int sizeFactor;
	Bool useMousepoll;

	// for random colors
	int colorcounter;
	float r;			// red value
	float g;			// green value
	float b;			// blue value
	float r2;			// red value
	float g2;			// green value
	float b2;			// blue value
	int colorrate;
	int skip;
	int atSkip;
	int speed;

	// Cursor data
	MouseCursor cursors[TEXTURES_SIZE];          // rolling array of cursors grabbed from x
	int lastTextureFilled;                       // last slot used in textures
	unsigned long lastCursorSerial;              // last cursor serial number grabbed from X
	int lastCursorIndex;                         // ...and its associated array index

	// Moved from drawParticles to get rid of spurious malloc's
	GLfloat *vertices_cache;
	int     vertex_cache_count;
	GLfloat *coords_cache;
	int     coords_cache_count;
	GLfloat *colors_cache;
	int     color_cache_count;


} ParticleSystem;


static int displayPrivateIndex = 0;

typedef struct _mousetrailsDisplay
{
	int  screenPrivateIndex;

	MousePollFunc *mpFunc;
}
mousetrailsDisplay;

typedef struct _mousetrailsScreen
{
	int posX;
	int posY;

	Bool active;

	ParticleSystem *ps;

	PositionPollingHandle pollHandle;

	PreparePaintScreenProc preparePaintScreen;
	DonePaintScreenProc    donePaintScreen;
	PaintOutputProc        paintOutput;
}
mousetrailsScreen;

static void
initParticles (int numParticles, ParticleSystem * ps)
{

	if (ps->particles)
		free(ps->particles);
	//ps->particles    = calloc(numParticles, sizeof(Particle));
	ps->particles    = calloc(TEXTURES_SIZE, sizeof(Particle));

	ps->numParticles = numParticles;
	ps->numDisplayedParticles = 0;
	ps->slowdown     = 1;
	ps->active       = FALSE;
	ps->lastx	     = 0;
	ps->lasty        = 0;
	ps->lastGen      = 0;
	ps->initialAlpha = 1;
	ps->sizeFactor   = 10;
	ps->colorcounter = 0;
	ps->x = 0;
	ps->y = 0;
	ps->skip = 1;
	ps->atSkip = 0;
	ps->speed = 1;
	ps->useMousepoll = 1;

	ps->lastTextureFilled = -1;
	ps->lastCursorIndex = 0;
	ps->lastCursorSerial = 0;

	// Initialize cache
	ps->vertices_cache      = NULL;
	ps->colors_cache        = NULL;
	ps->coords_cache        = NULL;
	ps->vertex_cache_count  = 0;
	ps->color_cache_count   = 0;
	ps->coords_cache_count  = 0;

	Particle *part = ps->particles;
	int i;
	for (i = 0; i < TEXTURES_SIZE; i++, part++){
		part->a = 0.0f;
		part->r = 0.0f;
		part->g = 0.0f;
		part->b = 0.0f;
	}


	for (i = 0; i < TEXTURES_SIZE; i++){
		ps->cursors[i].cursor_serial = 0;
		ps->cursors[i].xhot = 0;
		ps->cursors[i].yhot = 0;
		ps->cursors[i].width = 0;
		ps->cursors[i].height = 0;
	}

	for (i = 0; i < TEXTURES_SIZE; i++) glGenTextures(1, &ps->cursors[i].texture);

}


static void
drawParticles (CompScreen * s, ParticleSystem * ps)
{

	if (ps->numDisplayedParticles == 0) return;

	glEnable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnableClientState(GL_COLOR_ARRAY);
	glBlendFunc(GL_SRC_ALPHA, ps->blendMode);
	glEnable(GL_TEXTURE_2D);


	// Check that the caches are correct size 

	if (ps->vertex_cache_count != 1)
	{
		ps->vertices_cache =
			realloc(ps->vertices_cache,
					4 * 3 * sizeof(GLfloat));
		ps->vertex_cache_count = 1;
	}

	if (ps->coords_cache_count != 1)
	{
		ps->coords_cache =
			realloc(ps->coords_cache,
					4 * 2 * sizeof(GLfloat));
		ps->coords_cache_count = 1;
	}

	if (ps->color_cache_count  != 1)
	{
		ps->colors_cache =
			realloc(ps->colors_cache,
					4 * 4 * sizeof(GLfloat));
		ps->color_cache_count = 1;
	}


	GLfloat *vertices = ps->vertices_cache;
	GLfloat *coords   = ps->coords_cache;
	GLfloat *colors   = ps->colors_cache;

	int cornersSize = sizeof (GLfloat) * 8;
	int colorSize   = sizeof (GLfloat) * 4;

	GLfloat cornerCoords[8] = {0.0, 0.0,
		0.0, 1.0,
		1.0, 1.0,
		1.0, 0.0};

	int numActive = 0;

	Particle *part = ps->particles;
	int i;
	for (i = 0; i < ps->numParticles; i++, part++)
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

			if (ps->cursors[part->cursorIndex].texture)
			{
				glPushMatrix();

				glBindTexture(GL_TEXTURE_2D, ps->cursors[part->cursorIndex].texture);

				glTexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), ps->coords_cache);
				glVertexPointer(3, GL_FLOAT, 3 * sizeof(GLfloat), ps->vertices_cache);
				glColorPointer(4, GL_FLOAT, 4 * sizeof(GLfloat), ps->colors_cache);

				// draw particles
				glDrawArrays(GL_QUADS, 0, numActive);

				glPopMatrix();
			}

		}

	}


	glDisableClientState(GL_COLOR_ARRAY);
	glColor4usv(defaultColor);
	screenTexEnvMode(s, GL_REPLACE);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

}

static void
updateParticles (ParticleSystem * ps, float time)
{
	int i;
	Particle *part;
	float speed    = (time / 5000.0);

	ps->active = FALSE;

	part = ps->particles;

	int numDisplayedParticles = 0;

	for (i = 0; i < ps->numParticles; i++, part++)
	{
		if (part->a > 0.001f)
		{
			// modify opacity
			part->a -= 50 * part->a * part->fade * speed;
			ps->active  = TRUE;

			numDisplayedParticles++;
		}
	}
	ps->numDisplayedParticles = numDisplayedParticles;
}

static void
finiParticles (ParticleSystem * ps)
{
	free(ps->particles);
	if (ps->vertices_cache)
		free(ps->vertices_cache);
	if (ps->colors_cache)
		free(ps->colors_cache);
	if (ps->coords_cache)
		free(ps->coords_cache);

	int i;
	for (i = 0; i < TEXTURES_SIZE; i++) glDeleteTextures(1, &ps->cursors[i].texture);

}


/*
Window       root_return;
Window       child_return;
int          rootX, rootY;
int          winX, winY;
unsigned int maskReturn;
Bool         status;*/

static void
genNewParticles(CompScreen     *s,
		ParticleSystem *ps,
		int            time)
{


	ps->skip = mousetrailsGetSkip (s) ;
	ps->atSkip = (ps->atSkip + 1) % ps->skip;
	if (ps->atSkip != 0) return;

	MOUSETRAILS_SCREEN(s);

	Bool rColor     = mousetrailsGetRandom (s);
	//float life      = mousetrailsGetLife (s);
	int sizeFactor  = ps->sizeFactor;
	float threshold = ps->threshold;
	//float lifeNeg   = 1 - life;
	//float fadeExtra = 0.05f * (1.01 - life);
	int cursorIndex = 0;

	unsigned short *c = mousetrailsGetColor (s);

	float colr1 = (float)c[0] / 0xffff;
	float colg1 = (float)c[1] / 0xffff;
	float colb1 = (float)c[2] / 0xffff;
	//float cola  = (float)c[3] / 0xffff;

	float partw = ps->cursors[ps->lastCursorIndex].width;
	float parth = ps->cursors[ps->lastCursorIndex].height;

	float partoffw = ps->cursors[ps->lastCursorIndex].xhot;
	float partoffh = ps->cursors[ps->lastCursorIndex].yhot;

	int livex;
	int livey;

	// get x cursor position (rootX and rootY)
	/*if (!ss->ps->useMousepoll){
		status = XQueryPointer (s->display->display, s->root,
			    &root_return, &child_return,
			    &rootX, &rootY, &winX, &winY, &maskReturn);

		livex = rootX;
		livey = rootY;
	}
	else
	{*/
		livex = ss->posX;
		livey = ss->posY;
	//}

	int deltax = livex - ps->x;
	int deltay = livey - ps->y;
	int delta = deltax * deltax + deltay * deltay;
	ss->ps->x = livex;
	ss->ps->y = livey;
	//if (delta < threshold * threshold) return;
	if (delta < 1 ) return;

	int newx = (float)(ps->x + partw/2 - partoffw);
	int newy = (float)(ps->y + parth/2 - partoffh);


	Particle *part = ps->particles;
	int i;

	// possibly crashy
	if (mousetrailsGetNumParticles(s) != ps->numParticles){
	  //initParticles(mousetrailsGetNumParticles (s), ss->ps);
		ps->numParticles = mousetrailsGetNumParticles(s);
		ps->lastGen = -1;
			/*ps->slowdown = mousetrailsGetSlowdown (s);
			ps->initialAlpha = mousetrailsGetAlpha (s) ;
			ps->sizeFactor = mousetrailsGetSize (s) ;
			ps->threshold = mousetrailsGetThreshold (s) ;
			ps->colorrate = mousetrailsGetColorrate (s) ;*/
		for (i = 0; i < ps->numParticles; i++, part++){
			part->a = 0.0f;
			part->r = 0.0f;
			part->g = 0.0f;
			part->b = 0.0f;
		}
	}
	

	//if (delta > threshold * threshold) max_new = 1;

	int nextGen = (ps->lastGen + 1) % ps->numParticles;

	for (i = 0; i < ps->numParticles; i++, part++)
	{
		if (i == nextGen)
		{
			// record the particle number being generated
			ps->lastGen = i;

			cursorIndex = mousetrailsCursorUpdate(s);

			ps->slowdown = mousetrailsGetSlowdown (s);
			ps->initialAlpha = mousetrailsGetAlpha (s) ;
			ps->sizeFactor = mousetrailsGetSize (s) ;
			ps->threshold = mousetrailsGetThreshold (s) ;
			ps->colorrate = mousetrailsGetColorrate (s) ;

			// set the cursor array index
			part->cursorIndex = ps->lastCursorIndex = cursorIndex;

			part->fade = ps->slowdown / 10.0;

			// set size
			part->width = partw;
			part->height = parth;
			part->w_mod = part->h_mod = ((float)sizeFactor - 10.0) / 10.0;

			// set direction
			part->xd = newx - ps->lastx;
			part->yd = newy - ps->lasty;

			// set position
			part->x = newx;
			part->y = newy;
			ps->lastx = newx;
			ps->lasty = newy;
			part->z  = 0.0;


			if (rColor)
			{
				float value;
				// linear smoothing
				value = ps->colorcounter;
				// sinwave smoothing - nothing spectacular
				//value = (float)ps->colorrate - ((float)ps->colorrate / 2) * (1 + cos (3.14159 * (float)ps->colorcounter / (float)ps->colorrate));

				if (ps->colorcounter == 0)
				{
					ps->r2 = (float)(random() & 0xff) / 255;
					ps->g2 = (float)(random() & 0xff) / 255;
					ps->b2 = (float)(random() & 0xff) / 255;
				}
				ps->r = ((float)ps->colorrate - value)/(float)ps->colorrate * ps->r + value/(float)ps->colorrate * ps->r2;
				ps->g = ((float)ps->colorrate - value)/(float)ps->colorrate * ps->g + value/(float)ps->colorrate * ps->g2;
				ps->b = ((float)ps->colorrate - value)/(float)ps->colorrate * ps->b + value/(float)ps->colorrate * ps->b2;
				part->r = ps->r;
				part->g = ps->g;
				part->b = ps->b;
				ps->colorcounter = (ps->colorcounter + 1) % ps->colorrate;
			}
			else
			{
				part->r = colr1;
				part->g = colg1;
				part->b = colb1;
			}
			// set transparancy
			//part->a = ps->initialAlpha;
			float thresholdfactor = 1.0;
			if (delta < ps->threshold) thresholdfactor = (float)delta / (float)ps->threshold;
			part->a = ps->initialAlpha * thresholdfactor;

			ps->active = TRUE;
		}
	}
}

// damageRegion() seems to cause blockiness or jitters on the background window when scrolling, like on a webpage

static void
damageRegion (CompScreen *s)
{
	REGION   r;
	int      i;
	Particle *p;
	float    w, h, x1, x2, y1, y2;

	MOUSETRAILS_SCREEN (s);

	if (ss->ps->numDisplayedParticles == 0) return;

	if (!ss->ps)
		return;

	x1 = s->width;
	x2 = 0;
	y1 = s->height;
	y2 = 0;

	p = ss->ps->particles;

	for (i = 0; i < ss->ps->numParticles; i++, p++)
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

	r.rects = &r.extents;
	r.numRects = r.size = 1;

	r.extents.x1 = floor (x1);
	r.extents.x2 = ceil (x2);
	r.extents.y1 = floor (y1);
	r.extents.y2 = ceil (y2);

	damageScreenRegion (s, &r);
}


int mousetrailsCursorUpdate (CompScreen *s)
{
	MOUSETRAILS_SCREEN (s);

	Display * dpy = s->display->display;
	XFixesCursorImage *ci = XFixesGetCursorImage(dpy);

	/* Hack to avoid changing to an invisible (bugged)cursor image.
	 * Example: The animated firefox cursors.
	 */
	if (ci->width <= 1 && ci->height <= 1)
	{
		XFree (ci);
		return ss->ps->lastCursorIndex;
	}

	// only update cursor if necessary (instead of all the time)
	if (ci->cursor_serial == ss->ps->lastCursorSerial)
	{
		XFree (ci);
		return ss->ps->lastCursorIndex;
	}

	// see if cursor already exists in textures
	int i;
	for (i = 0; i < TEXTURES_SIZE; i++){
		if (ci->cursor_serial == ss->ps->cursors[i].cursor_serial){
			ss->ps->lastCursorSerial = ci->cursor_serial;
			XFree (ci);
			return i;
		}
	}

	// otherwise grab the new cursor into textures
	int fillTexture = (ss->ps->lastTextureFilled + 1) % TEXTURES_SIZE;

	unsigned char *pixels = malloc(ci->width * ci->height * 4);

	for (i = 0; i < ci->width * ci->height; i++)
	{
		unsigned long pix = ci->pixels[i];
		pixels[i * 4] = pix & 0xff;
		pixels[(i * 4) + 1] = (pix >> 8) & 0xff;
		pixels[(i * 4) + 2] = (pix >> 16) & 0xff;
		pixels[(i * 4) + 3] = (pix >> 24) & 0xff;
	}

	glBindTexture(GL_TEXTURE_2D, ss->ps->cursors[fillTexture].texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ci->width, ci->height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	// save data for this cursor
	ss->ps->cursors[fillTexture].cursor_serial = ss->ps->lastCursorSerial = ci->cursor_serial;
	ss->ps->cursors[fillTexture].xhot = ci->xhot;
	ss->ps->cursors[fillTexture].yhot = ci->yhot;
	ss->ps->cursors[fillTexture].width = ci->width;
	ss->ps->cursors[fillTexture].height = ci->height;
	ss->ps->lastTextureFilled = fillTexture;

	XFree (ci);
	free (pixels);

	return fillTexture;
}


static void
positionUpdate (CompScreen *s,
		int        x,
		int        y)
{
	MOUSETRAILS_SCREEN (s);

	ss->posX = x;
	ss->posY = y;
}



static void
mousetrailsPreparePaintScreen (CompScreen *s,
		int        time)
{

	MOUSETRAILS_SCREEN (s);
	MOUSETRAILS_DISPLAY (s->display);

	//Bool useMousepoll = mousetrailsGetMousepoll (s);

	if (ss->active && !ss->pollHandle/* && useMousepoll*/)
	{
		(*sd->mpFunc->getCurrentPosition) (s, &ss->posX, &ss->posY);
		ss->pollHandle = (*sd->mpFunc->addPositionPolling) (s, positionUpdate);
	}

	if (ss->active && !ss->ps)
	{
		ss->ps = calloc(1, sizeof(ParticleSystem));
		if (!ss->ps)
		{
			UNWRAP (ss, s, preparePaintScreen);
			(*s->preparePaintScreen) (s, time);
			WRAP (ss, s, preparePaintScreen, mousetrailsPreparePaintScreen);
			return;
		}
		initParticles(mousetrailsGetNumParticles (s), ss->ps);

		ss->ps->slowdown = mousetrailsGetSlowdown (s);
		ss->ps->initialAlpha = mousetrailsGetAlpha (s) ;
		ss->ps->threshold = mousetrailsGetThreshold (s) ;
		ss->ps->colorrate = mousetrailsGetColorrate (s) ;
		ss->ps->sizeFactor = mousetrailsGetSize (s) ;
		ss->ps->skip = mousetrailsGetSkip (s) ;
		ss->ps->blendMode = GL_ONE_MINUS_SRC_ALPHA;
		//ss->ps->useMousepoll = mousetrailsGetMousepoll (s);

		mousetrailsCursorUpdate(s);

	}


	if (ss->ps && ss->ps->active)
	{
		updateParticles (ss->ps, time);
		damageRegion (s);
	}

	if (ss->ps && ss->active)
		genNewParticles (s, ss->ps, time);

	UNWRAP (ss, s, preparePaintScreen);
	(*s->preparePaintScreen) (s, time);
	WRAP (ss, s, preparePaintScreen, mousetrailsPreparePaintScreen);

}


static void
mousetrailsDonePaintScreen (CompScreen *s)
{
	MOUSETRAILS_SCREEN (s);
	MOUSETRAILS_DISPLAY (s->display);

	if (ss->active || (ss->ps && ss->ps->active))
		damageRegion (s);

	if (!ss->active && ss->pollHandle && ss->ps->useMousepoll)
	{
		(*sd->mpFunc->removePositionPolling) (s, ss->pollHandle);
		ss->pollHandle = 0;
	}

	if (!ss->active && ss->ps && !ss->ps->active)
	{
		finiParticles (ss->ps);
		free (ss->ps);
		ss->ps = NULL;
	}

	UNWRAP (ss, s, donePaintScreen);
	(*s->donePaintScreen) (s);
	WRAP (ss, s, donePaintScreen, mousetrailsDonePaintScreen);
}

static Bool
mousetrailsPaintOutput (CompScreen              *s,
		const ScreenPaintAttrib *sa,
		const CompTransform     *transform,
		Region                  region,
		CompOutput              *output,
		unsigned int            mask)
{
	Bool           status;
	CompTransform  sTransform;

	MOUSETRAILS_SCREEN (s);

	UNWRAP (ss, s, paintOutput);
	status = (*s->paintOutput) (s, sa, transform, region, output, mask);
	WRAP (ss, s, paintOutput, mousetrailsPaintOutput);

	

	if (!ss->ps || !ss->ps->active)
		return status;

// this doesn't seem to work
//	if (mask & PAINT_SCREEN_TRANSFORMED_MASK) return FALSE;

	matrixGetIdentity (&sTransform);

	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

	glPushMatrix ();
	glLoadMatrixf (sTransform.m);

	drawParticles (s, ss->ps);

	glPopMatrix();

	glColor4usv (defaultColor);

	return status;
}

static Bool
mousetrailsTerminate (CompDisplay     *d,
		CompAction      *action,
		CompActionState state,
		CompOption      *option,
		int             nOption)
{
	CompScreen *s;
	Window     xid;

	xid = getIntOptionNamed (option, nOption, "root", 0);

	s = findScreenAtDisplay (d, xid);
	if (s)
	{
		MOUSETRAILS_SCREEN (s);

		ss->active = FALSE;
		damageRegion (s);

		return TRUE;
	}
	return FALSE;
}

static Bool
mousetrailsInitiate (CompDisplay     *d,
		CompAction      *action,
		CompActionState state,
		CompOption      *option,
		int             nOption)
{


	CompScreen *s;
	Window     xid;

	xid    = getIntOptionNamed (option, nOption, "root", 0);

	s = findScreenAtDisplay (d, xid);
	if (s)
	{
		MOUSETRAILS_SCREEN (s);

		if (ss->active)
			return mousetrailsTerminate (d, action, state, option, nOption);

		ss->active = TRUE;

		return TRUE;
	}
	return FALSE;
}


static Bool
mousetrailsInitScreen (CompPlugin *p,
		CompScreen *s)
{

	MOUSETRAILS_DISPLAY (s->display);

	mousetrailsScreen *ss = (mousetrailsScreen *) calloc (1, sizeof (mousetrailsScreen) );

	if (!ss)
		return FALSE;

	s->base.privates[sd->screenPrivateIndex].ptr = ss;

	WRAP (ss, s, paintOutput, mousetrailsPaintOutput);
	WRAP (ss, s, preparePaintScreen, mousetrailsPreparePaintScreen);
	WRAP (ss, s, donePaintScreen, mousetrailsDonePaintScreen);

	ss->active = TRUE;

	ss->pollHandle = 0;

	ss->ps  = NULL;

	return TRUE;
}


static void
mousetrailsFiniScreen (CompPlugin *p,
		CompScreen *s)
{
	MOUSETRAILS_SCREEN (s);
	MOUSETRAILS_DISPLAY (s->display);

	//Restore the original function
	UNWRAP (ss, s, paintOutput);
	UNWRAP (ss, s, preparePaintScreen);
	UNWRAP (ss, s, donePaintScreen);

	if (ss->pollHandle && ss->ps->useMousepoll)
		(*sd->mpFunc->removePositionPolling) (s, ss->pollHandle);

	if (ss->ps && ss->ps->active)
		damageScreen (s);

	//Free the pointer
	free (ss);
}

static Bool
mousetrailsInitDisplay (CompPlugin  *p,
		CompDisplay *d)
{

	//Generate a mousetrails display
	mousetrailsDisplay *sd;
	int              index;

	if (!checkPluginABI ("core", CORE_ABIVERSION) ||
			!checkPluginABI ("mousepoll", MOUSEPOLL_ABIVERSION))
		return FALSE;

	if (!getPluginDisplayIndex (d, "mousepoll", &index))
		return FALSE;

	sd = (mousetrailsDisplay *) malloc (sizeof (mousetrailsDisplay));

	if (!sd)
		return FALSE;

	//Allocate a private index
	sd->screenPrivateIndex = allocateScreenPrivateIndex (d);

	//Check if its valid
	if (sd->screenPrivateIndex < 0)
	{
		//Its invalid so free memory and return
		free (sd);
		return FALSE;
	}

	sd->mpFunc = d->base.privates[index].ptr;

	mousetrailsSetInitiateInitiate (d, mousetrailsInitiate);
	mousetrailsSetInitiateTerminate (d, mousetrailsTerminate);

	//Record the display
	d->base.privates[displayPrivateIndex].ptr = sd;
	return TRUE;
}

static void
mousetrailsFiniDisplay (CompPlugin  *p,
		CompDisplay *d)
{
	MOUSETRAILS_DISPLAY (d);
	//Free the private index
	freeScreenPrivateIndex (d, sd->screenPrivateIndex);
	//Free the pointer
	free (sd);
}



static Bool
mousetrailsInit (CompPlugin * p)
{
	displayPrivateIndex = allocateDisplayPrivateIndex();

	if (displayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static void
mousetrailsFini (CompPlugin * p)
{
	if (displayPrivateIndex >= 0)
		freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
mousetrailsInitObject (CompPlugin *p,
		CompObject *o)
{
	static InitPluginObjectProc dispTab[] = {
		(InitPluginObjectProc) 0, /* InitCore */
		(InitPluginObjectProc) mousetrailsInitDisplay,
		(InitPluginObjectProc) mousetrailsInitScreen
	};

	RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
mousetrailsFiniObject (CompPlugin *p,
		CompObject *o)
{
	static FiniPluginObjectProc dispTab[] = {
		(FiniPluginObjectProc) 0, /* FiniCore */
		(FiniPluginObjectProc) mousetrailsFiniDisplay,
		(FiniPluginObjectProc) mousetrailsFiniScreen
	};

	DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable mousetrailsVTable = {
	"mousetrails",
	0,
	mousetrailsInit,
	mousetrailsFini,
	mousetrailsInitObject,
	mousetrailsFiniObject,
	0,
	0
};

CompPluginVTable *
getCompPluginInfo (void)
{
	return &mousetrailsVTable;
}
