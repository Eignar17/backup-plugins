/*
 * Compiz ring/pie menu plugin
 *
 * ringmenu.c
 *
 * Copyright : (C) 2010 by Matvey "blackhole89" Soloviev
 * E-mail    : blackhole89@gmail.com
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

#include <math.h>
#include <memory.h>
#include <string.h>

#include <compiz-core.h>
#include <compiz-mousepoll.h>

#include "ringmenu_options.h"

#define GET_RINGMENU_DISPLAY(d)						          \
	((RingmenuDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define RINGMENU_DISPLAY(d)					  \
	RingmenuDisplay *sd = GET_RINGMENU_DISPLAY (d)

#define GET_RINGMENU_SCREEN(s, sd)						           \
	((RingmenuScreen *) (s)->base.privates[(sd)->screenPrivateIndex].ptr)

#define RINGMENU_SCREEN(s)						                              \
	RingmenuScreen *ss = GET_RINGMENU_SCREEN (s, GET_RINGMENU_DISPLAY (s->display))

typedef struct {
	int  screenPrivateIndex;

	MousePollFunc *mpFunc;
} RingmenuDisplay;

typedef struct {
	float start;
	float end;
	char *ident;
	char *command;
	Bool active;
} RingmenuEntry;

typedef struct {
	int posX;
	int posY;
	int mouseX;
	int mouseY;

	Bool active;

	PositionPollingHandle pollHandle;
	
	PreparePaintScreenProc  preparePaintScreen;
	DonePaintScreenProc	donePaintScreen;
	PaintOutputProc		paintOutput;

	RingmenuEntry *ent;
	int nent;

	Bool isinrange;
	float angle;
} RingmenuScreen;

#include "liststruct.h"

static int displayPrivateIndex = 0;

static void
positionUpdate (CompScreen *s,
		int        x,
		int        y)
{
	RINGMENU_SCREEN (s);
	
	ss->mouseX=x;
	ss->mouseY=y;
    
	if(ss->active) {
		float k,l;
		x-=ss->posX;
		y-=ss->posY;
		if((k=sqrt(x*x+y*y))>135.0) {
			l=135.0/k;
			x*=l;
			y*=l;
		}
		if(k>100.0) {
			ss->isinrange=TRUE;
		} else {
			ss->isinrange=FALSE;
		}
		ss->angle=atan2(y,x)/(2*3.14159265);
		if(ss->angle<0) ss->angle+=1.0;
		x+=ss->posX;
		y+=ss->posY;
		warpPointer(s,x-ss->mouseX,y-ss->mouseY);
	}

	ss->mouseX = x;
	ss->mouseY = y;
}

static void
ringmenuPreparePaintScreen (CompScreen *s,
				 int	    time)
{
	RINGMENU_SCREEN (s);
	RINGMENU_DISPLAY (s->display);

	if (ss->active && !ss->pollHandle)
	{
		(*sd->mpFunc->getCurrentPosition) (s, &ss->posX, &ss->posY);
		ss->pollHandle = (*sd->mpFunc->addPositionPolling) (s, positionUpdate);
	
		REGION   r;
		r.rects=&r.extents;
		r.numRects=r.size=1;
		r.extents.x1=ss->posX-150;
		r.extents.x2=ss->posX+150;
		r.extents.y1=ss->posY-150;
		r.extents.y2=ss->posY+150;
		damageScreenRegion(s,&r);
	//	damageScreen(s);
	}

	UNWRAP (ss, s, preparePaintScreen);
	(*s->preparePaintScreen) (s, time);
	WRAP (ss, s, preparePaintScreen, ringmenuPreparePaintScreen);
}

static void
ringmenuDonePaintScreen (CompScreen *s)
{
	RINGMENU_SCREEN (s);
	RINGMENU_DISPLAY (s->display);

//	if (ss->active || (ss->ps && ss->ps->active))
//		damageRegion (s);
	
	if(ss->active) {
		REGION   r;
		r.rects=&r.extents;
		r.numRects=r.size=1;
		r.extents.x1=ss->posX-150;
		r.extents.x2=ss->posX+150;
		r.extents.y1=ss->posY-150;
		r.extents.y2=ss->posY+150;
		damageScreenRegion(s,&r);
	}
	//damageScreen(s);

	if (!ss->active && ss->pollHandle)
	{
		(*sd->mpFunc->removePositionPolling) (s, ss->pollHandle);
		ss->pollHandle = 0;
	}

	UNWRAP (ss, s, donePaintScreen);
	(*s->donePaintScreen) (s);
	WRAP (ss, s, donePaintScreen, ringmenuDonePaintScreen);
}

void drawRingSegment(double anglefrom,double angleto,double rfrom,double rto);

void drawRingSegment(double anglefrom,double angleto,double rfrom,double rto)
{
	for(double k=anglefrom;k<(angleto-0.01);k+=0.01) {
		glVertex2d((rfrom*cos(k)),(rfrom*sin(k)));
		glVertex2d((rfrom*cos(k+0.01)),(rfrom*sin(k+0.01)));
		glVertex2d((rto*cos(k+0.01)),(rto*sin(k+0.01)));
		glVertex2d((rto*cos(k)),(rto*sin(k)));
	}
}

void drawIdent(char* ident, float angle, Bool active);
{
	glPushMatrix();

	glTranslated(118.*cos(angle),118.*sin(angle),0);
	glScaled(1.5,1.5,0);

	if(!strcmp(ident,"next")) {
		glColor4f(0,0,0,0.7);
		glBegin(GL_TRIANGLES);
			glVertex2d(-10,-18);
			glVertex2d(-10,18);
			glVertex2d(24,0);
		glEnd();
		glBlendFunc(GL_ONE,GL_ONE);
		glColor4f(active?0.8:0.5,active?0.8:0.5,0.0,0.3);
		glBegin(GL_TRIANGLES);
			glVertex2d(-6,-12);
			glVertex2d(-6,12);
			glVertex2d(16,0);
		glEnd();
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	} else if(!strcmp(ident,"stop")) {
		glColor4f(0,0,0,0.7);
		glBegin(GL_QUADS);
			glVertex2d(-18,-18);
			glVertex2d(-18,18);
			glVertex2d(18,18);
			glVertex2d(18,-18);
		glEnd();
		glBlendFunc(GL_ONE,GL_ONE);
		glColor4f(active?0.8:0.5,0.0,0.0,0.3);
		glBegin(GL_QUADS);
			glVertex2d(-14,-14);
			glVertex2d(-14,14);
			glVertex2d(14,14);
			glVertex2d(14,-14);
		glEnd();
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	} else if(!strcmp(ident,"pause")) {
		glColor4f(0,0,0,0.7);
		glBegin(GL_QUADS);
			glVertex2d(-18,-18);
			glVertex2d(-18,18);
			glVertex2d(18,18);
			glVertex2d(18,-18);
		glEnd();
		glBlendFunc(GL_ONE,GL_ONE);
		glColor4f(0.0,active?0.8:0.5,0.0,0.3);
		glBegin(GL_QUADS);
			glVertex2d(-14,-14);
			glVertex2d(-14,14);
			glVertex2d(-3,14);
			glVertex2d(-3,-14);
			glVertex2d(2,-14);
			glVertex2d(2,14);
			glVertex2d(14,14);
			glVertex2d(14,-14);
		glEnd();
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	} else if(!strcmp(ident,"gedit")) {
		glColor4f(0,0,0,0.7);
		glBegin(GL_QUADS);
			glVertex2d(-18,-18);
			glVertex2d(-18,18);
			glVertex2d(18,18);
			glVertex2d(18,-18);
		glEnd();
		glBlendFunc(GL_ONE,GL_ONE);
		glColor4f(active?0.8:0.5,active?0.8:0.5,active?0.8:0.5,0.3);
		glBegin(GL_QUADS);
			glVertex2d(-14,-14);
			glVertex2d(-14,14);
			glVertex2d(14,14);
			glVertex2d(14,-14);
		glEnd();
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0,0,0,0.7);
		glBegin(GL_QUADS);
			glVertex2d(-10,-10);
			glVertex2d(-10,-8);
			glVertex2d(10,-8);
			glVertex2d(10,-10);
			glVertex2d(-10,-4);
			glVertex2d(-10,-2);
			glVertex2d(10,-2);
			glVertex2d(10,-4);
		glEnd();
	}

	glPopMatrix();
}

static Bool
ringmenuPaintOutput (CompScreen		      *s,
			  const ScreenPaintAttrib *sa,
			  const CompTransform	 *transform,
			  Region	              region,
			  CompOutput	          *output,
			  unsigned int	        mask)
{
	Bool	       status;
	CompTransform  sTransform;

	RINGMENU_SCREEN (s);

	UNWRAP (ss, s, paintOutput);
	status = (*s->paintOutput) (s, sa, transform, region, output, mask);
	WRAP (ss, s, paintOutput, ringmenuPaintOutput);

	if(!ss->active) return status;

	matrixGetIdentity(&sTransform);
	transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

	screenTexEnvMode(s, GL_MODULATE);

	glPushMatrix();
	glLoadMatrixf (sTransform.m);

	glColor4f(0.0,0.0,0.0,0.5);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);

	glTranslated(ss->posX,ss->posY,0);
	
	glBegin(GL_QUADS);
		drawRingSegment(0,2*3.1415926,102,134);
	glEnd();

	glColor4f(1.0,1.0,1.0,0.5);
	glBegin(GL_QUADS);
		for(int i=0;i<ss->nent;++i) {
			drawRingSegment((ss->ent[i].start+0.003)*2*3.14159265,(ss->ent[i].end-0.003)*2*3.14159265,106,130);
		}
	glEnd();
	glColor4f(1.0,1.0,1.0,0.2);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glBegin(GL_QUADS);
		for(int i=0;i<ss->nent;++i) {
			ss->ent[i].active=FALSE;
			if(ss->isinrange)
				if(ss->ent[i].start<ss->angle && ss->angle<ss->ent[i].end) {
					ss->ent[i].active=TRUE;
					drawRingSegment((ss->ent[i].start+0.001)*2*3.14159265,(ss->ent[i].end-0.001)*2*3.14159265,105,131);
				}
		}
//		drawRingSegment(0,3.1415926,106,130);
	glEnd();
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	for(int i=0;i<ss->nent;++i) {
		drawIdent(ss->ent[i].ident,(ss->ent[i].start+ss->ent[i].end)*3.14159265,ss->ent[i].active);
	}

	glPopMatrix();

	glColor4usv(defaultColor);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	screenTexEnvMode(s, GL_REPLACE);

	return status;

	//matrixGetIdentity (&sTransform);

	//transformToScreenSpace (s, output, -DEFAULT_Z_CAMERA, &sTransform);

	//glLoadMatrixf (sTransform.m);
}


/* INITIATE */
static Bool
ringmenuInitiate (CompDisplay	 *d,
		   CompAction	  *action,
		   CompActionState state,
		   CompOption	  *option,
		   int		     nOption)
{
	CompScreen *s;
	Window	 xid;

	xid	= getIntOptionNamed (option, nOption, "root", 0);

	s = findScreenAtDisplay (d, xid);
	if (s)
	{
		RINGMENU_SCREEN (s);
		RINGMENU_DISPLAY (d);

		if (state & CompActionStateInitButton)
			action->state |= CompActionStateTermButton;	
	
		//store current pos.	
		(*sd->mpFunc->getCurrentPosition) (s, &ss->mouseX, &ss->mouseY);	
		ss->posX=ss->mouseX;
		ss->posY=ss->mouseY;
		ss->active = TRUE;
		ss->isinrange = FALSE;

		damageScreen(s);
	
		return TRUE;
	}
	return FALSE;
}

static Bool
ringmenuTerminate (CompDisplay     *d,
		    CompAction      *action,
		    CompActionState state,
		    CompOption      *option,
		    int             nOption)
{
	CompScreen *s;
	Window     xid;

	xid = getIntOptionNamed (option, nOption, "root", 0);

	s = findScreenAtDisplay (d, xid);
	if(s) {
		RINGMENU_SCREEN(s);
		RINGMENU_DISPLAY(d);
		
		if(!ss->active) return TRUE;
		
		(*sd->mpFunc->getCurrentPosition) (s, &ss->mouseX, &ss->mouseY);
		warpPointer(s, ss->posX-ss->mouseX, ss->posY-ss->mouseY);		

		if(ss->isinrange) {
			for(int i=0;i<ss->nent;++i) {
				if(ss->ent[i].start<ss->angle && ss->angle<ss->ent[i].end) {
					system(ss->ent[i].command);
				}
			}
		}
	
		ss->active = FALSE;
	}
	return TRUE;
}

/* FURTHER INIT/FINI*/
static void
initEntry (void *object,
           void *closure)
{
    CompScreen          *s = (CompScreen *) closure;
    RingmenuEntry    *ent  = (RingmenuEntry *) object;
}

static void
finiEntry (void *object,
           void *closure)
{

}

/* Installed as a handler for the images setting changing through bcop */
static void
ringmenuEntriesChanged (CompScreen             *s,
	                CompOption             *o,
                        RingmenuScreenOptions num)
{
	RINGMENU_SCREEN(s);
	
	int nent;
	ss->ent = processMultiList(sizeof (RingmenuEntry),
				 ss->ent, &nent,
				 initEntry, finiEntry, s, 4,
				 ringmenuGetOptStartOption (s),
				 offsetof (RingmenuEntry, start),
				 ringmenuGetOptEndOption (s),
				 offsetof (RingmenuEntry, end),
				 ringmenuGetOptIdentOption (s),
				 offsetof (RingmenuEntry, ident),
				 ringmenuGetOptCommandOption (s),
				 offsetof (RingmenuEntry, command)
				 );
	ss->nent = nent;
}

static Bool
ringmenuInitScreen (CompPlugin *p,
			 CompScreen *s)
{
	RINGMENU_DISPLAY (s->display);

	RingmenuScreen *ss = (RingmenuScreen *) calloc (1, sizeof (RingmenuScreen) );

	if (!ss)
	return FALSE;

	s->base.privates[sd->screenPrivateIndex].ptr = ss;

	WRAP (ss, s, paintOutput, ringmenuPaintOutput);
	WRAP (ss, s, preparePaintScreen, ringmenuPreparePaintScreen);
	WRAP (ss, s, donePaintScreen, ringmenuDonePaintScreen);

	ringmenuSetOptStartNotify(s, ringmenuEntriesChanged);
	ringmenuSetOptEndNotify(s, ringmenuEntriesChanged);
	ringmenuSetOptIdentNotify(s, ringmenuEntriesChanged);
	ringmenuSetOptCommandNotify(s, ringmenuEntriesChanged);

	ss->active = FALSE;

	ss->pollHandle = 0;

	return TRUE;
}


static void
ringmenuFiniScreen (CompPlugin *p, CompScreen *s)
{
	RINGMENU_SCREEN (s);
	RINGMENU_DISPLAY (s->display);

	//Restore the original function
	UNWRAP (ss, s, paintOutput);
	UNWRAP (ss, s, preparePaintScreen);
	UNWRAP (ss, s, donePaintScreen);

	if (ss->pollHandle)
		(*sd->mpFunc->removePositionPolling) (s, ss->pollHandle);

//	if (ss->ps && ss->ps->active)
//		damageScreen (s);

	//Free the pointer
	free (ss);
}

static Bool
ringmenuInitDisplay (CompPlugin  *p, CompDisplay *d)
{
	//Generate a ringmenu display
	RingmenuDisplay *sd;
	int  index;

	if (!checkPluginABI ("core", CORE_ABIVERSION) ||
		!checkPluginABI ("mousepoll", MOUSEPOLL_ABIVERSION))
		return FALSE;

	if (!getPluginDisplayIndex (d, "mousepoll", &index))
		return FALSE;

	sd = (RingmenuDisplay *) malloc (sizeof (RingmenuDisplay));

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

	ringmenuSetInitiateInitiate (d, ringmenuInitiate);
	ringmenuSetInitiateTerminate (d, ringmenuTerminate);
	ringmenuSetInitiateButtonInitiate (d, ringmenuInitiate);
	ringmenuSetInitiateButtonTerminate (d, ringmenuTerminate);

	//Record the display
	d->base.privates[displayPrivateIndex].ptr = sd;
	return TRUE;
}

static void
ringmenuFiniDisplay (CompPlugin *p, CompDisplay *d)
{
	RINGMENU_DISPLAY(d);
	//Free the private index
	freeScreenPrivateIndex (d, sd->screenPrivateIndex);
	//Free the pointer
	free (sd);
}




/* INIT/FINI */
static Bool
ringmenuInit (CompPlugin * p)
{
	displayPrivateIndex = allocateDisplayPrivateIndex();

	if (displayPrivateIndex < 0)
		return FALSE;

	return TRUE;
}

static void
ringmenuFini (CompPlugin * p)
{
	if (displayPrivateIndex >= 0)
	freeDisplayPrivateIndex (displayPrivateIndex);
}

static CompBool
ringmenuInitObject (CompPlugin *p,
			 CompObject *o)
{
	static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) ringmenuInitDisplay,
	(InitPluginObjectProc) ringmenuInitScreen
	};

	RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
ringmenuFiniObject (CompPlugin *p,
			 CompObject *o)
{
	static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) ringmenuFiniDisplay,
	(FiniPluginObjectProc) ringmenuFiniScreen
	};

	DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

CompPluginVTable ringmenuVTable = {
	"ringmenu",
	0,
	ringmenuInit,
	ringmenuFini,
	ringmenuInitObject,
	ringmenuFiniObject,
	0,
	0
};

CompPluginVTable *
getCompPluginInfo (void)
{
	return &ringmenuVTable;
}

