/*
 *
 * Compiz smackpad plugin
 * smackpad.c
 * Copyright (c) 2007 Eugen Feller <eugen.feller@uni-duesseldorf.de>
 *
 * Client message to root window taken from mswitch.c:
 * Copyright : (c) 2007 Robert Carr
 * E-mail    : racarr@opencompositing.org
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
 * Thanks to Michele Campeotto <micampe@micampe.it> and 
 * Fernando Herrera <fherrera@onirica.com> for their ideas 
 * which helped me to make this plugin.
 */

#include <compiz-core.h>
#include "smackpad_options.h"
#include <X11/Xlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define POSITION_FILE "/sys/devices/platform/hdaps/position"
#define CALIBRATE_FILE "/sys/devices/platform/hdaps/calibrate"
#define INVERT_FILE "/sys/devices/platform/hdaps/invert"

typedef enum
{
	LEFT,
	RIGHT,
	UP,
	DOWN
}smackDirection;

typedef enum
{
	CALIBRATE,
	POSITION
}smackpadMisc;

typedef struct 
{
	int x;
	int y;
}smackpadPosition;

typedef struct
{	
	int pitchX;
	int pitchY;
	int stableX;
	int stableY;
	int aPitchX;
	int aPitchY;
}smackpadData;

static int sensitivityRate=5; 
pthread_t readPositionThread; 
static int readPositionThreadStatus=-1; 
static Bool statusLoop=FALSE; 
static Bool statusPlugin=FALSE;
static CompScreen *screen=NULL;

/* prototype definitions */

static void smackpadRotateDesktop (smackDirection direction);
static void* smackpadGetPosition (void *data);
static void smackpadRotate (int dX, int dY);
static Bool smackpadDetectInvertedAxes (void);
static void smackpadRotateHelperLeftRight (int pitchX, Bool statusAxes);
static void smackpadRotateHelperUpDown (int pitchY, Bool statusAxes);
static void smackpadUpdateSensitivityRate (CompDisplay *d, CompOption *opt, SmackpadDisplayOptions num);
static void smackpadCalculateData (smackpadData *data, smackpadPosition currentPosition, smackpadPosition initialPosition);
static Bool smackpadInitiate (CompDisplay *d, CompAction *ac, CompActionState state, CompOption *option, int nOption);
static Bool smackpadReadHDAPSData (smackpadMisc misc, Bool invert, smackpadPosition *position);
static Bool smackpadInitHDAPS (void);
static CompBool smackpadInitObject (CompPlugin *p, CompObject *o);
static Bool smackpadInitDisplay (CompPlugin *p, CompDisplay *d);
static Bool smackpadInitScreen (CompPlugin *p, CompScreen *s);
static void smackpadFini (CompPlugin *p);

static Bool 
smackpadReadHDAPSData (smackpadMisc misc,
					   Bool invert,
					   smackpadPosition *position)
{
	char buf[255];
	FILE *file=NULL;
	
	switch(misc)
	{
		case CALIBRATE:
			file = fopen(CALIBRATE_FILE, "r");
			break;
		case POSITION:
			file = fopen(POSITION_FILE, "r");
			break;
	}
	
	if (file == NULL)
		return FALSE;
	
	fread(buf, 255, 1, file);
  	fclose(file);
	
	if(invert)
		sscanf(buf, "(%d,%d)", &position->y, &position->x);
	else
		sscanf(buf, "(%d,%d)", &position->x, &position->y);
	
	return TRUE;
	
}

static Bool 
smackpadDetectInvertedAxes (void)
{
	char buf[255];
	int status;
	FILE *file=NULL;
	
	file = fopen(INVERT_FILE, "r");

	if (file == NULL)
		return FALSE;
	
	fread(buf, 255, 1, file);
  	fclose(file);
	
	sscanf(buf, "%d", &status);
	
	if(status)
		return TRUE;
	
	return FALSE;
}

static void 
smackpadRotate (int dX,
				int dY)
{
	XEvent xev;
	xev.xclient.type = ClientMessage;
	xev.xclient.display = screen->display->display;
	xev.xclient.format = 32;
	xev.xclient.message_type = screen->display->desktopViewportAtom;
	xev.xclient.window = screen->root;
	xev.xclient.data.l[0] = (screen->x+dX)*screen->width;
	xev.xclient.data.l[1] = (screen->y+dY)*screen->height;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 0;
	xev.xclient.data.l[4] = 0;
	
	XSendEvent(screen->display->display, screen->root, FALSE, 
		   SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

static void 
smackpadRotateDesktop (smackDirection direction)
{	
	switch(direction)
	{
		case LEFT:
				smackpadRotate(-1,0);
				break;
		case RIGHT:
				smackpadRotate(1,0);
				break;
		case UP:
				smackpadRotate(0,-1);
				break;
		case DOWN:
				smackpadRotate(0,1);
				break;
	}
}

static void* 
smackpadGetPosition (void *misc)
{	
	struct timespec sleepTimer;
	smackpadPosition currentPosition;
	smackpadPosition initialPosition;
	smackpadData data = {0,0,0,0,0,0};

	sleepTimer.tv_sec=0;
	sleepTimer.tv_nsec=20000000; /* 20ms */
	
	Bool statusAxes=smackpadDetectInvertedAxes();

	if(!smackpadReadHDAPSData(CALIBRATE,statusAxes,&currentPosition)) return NULL;
	
	initialPosition.x=currentPosition.x;
	initialPosition.y=currentPosition.y;
	
	while (statusLoop)
	{
		if(!smackpadReadHDAPSData(POSITION,statusAxes,&currentPosition)) return NULL;
		
		smackpadCalculateData(&data,currentPosition,initialPosition);
		
		if(data.aPitchX > sensitivityRate && data.stableX > 30)
		{
			smackpadRotateHelperLeftRight(data.pitchX,statusAxes);
			data.stableX=0;
		}
		else if(data.aPitchY > sensitivityRate && data.stableY > 30)
		{
			smackpadRotateHelperUpDown(data.pitchY,statusAxes);
			data.stableY=0;
		}
		
		nanosleep(&sleepTimer,NULL);
	}
	
	return NULL;
}

static void 
smackpadCalculateData (smackpadData *data, 
					   smackpadPosition currentPosition,
					   smackpadPosition initialPosition)
{

		data->pitchX=currentPosition.x-initialPosition.x;
		data->pitchY=currentPosition.y-initialPosition.y;
		data->aPitchX=abs(data->pitchX);
		data->aPitchY=abs(data->pitchY);
		
		if(data->aPitchX < 5)
			data->stableX+=1;
		if(data->aPitchY < 5)
			data->stableY+=1;	
}

static void 
smackpadRotateHelperLeftRight (int pitchX,
							   Bool statusAxes)
{
			smackDirection leftDirection =  (!statusAxes) ? RIGHT : LEFT;
			smackDirection rightDirection =	(!statusAxes) ? LEFT : RIGHT;
			
			if(pitchX<0)
				smackpadRotateDesktop(leftDirection);
			else
				smackpadRotateDesktop(rightDirection);
}

static void 
smackpadRotateHelperUpDown (int pitchY,
							Bool statusAxes)
{
			smackDirection upDirection = (!statusAxes) ? DOWN : UP;
			smackDirection downDirection = (!statusAxes) ? UP: DOWN;
			
			if(pitchY<0)
				smackpadRotateDesktop(upDirection);
			else
				smackpadRotateDesktop(downDirection);	
}

static void 
smackpadUpdateSensitivityRate (CompDisplay *d,
							   CompOption *opt,
							   SmackpadDisplayOptions num)
{
	sensitivityRate=smackpadGetSensitivityRate(d);
}

static Bool 
smackpadInitHDAPS (void) 
{
	FILE *file=NULL;
	file=fopen(POSITION_FILE,"r");
	
	if(file!=NULL)
	{
		fclose(file);
		return TRUE;
	}
	
	return FALSE;
}

static Bool 
smackpadInitiate (CompDisplay *d,
				  CompAction *ac,
				  CompActionState state,
				  CompOption *option,
				  int nOption)
{	
	Bool testHdapsStatus=smackpadInitHDAPS();
	
	if(testHdapsStatus && !statusPlugin)
	{
		statusLoop=TRUE;
		statusPlugin=TRUE;
		readPositionThreadStatus=pthread_create(&readPositionThread,NULL,smackpadGetPosition,NULL);
	}
	else if(testHdapsStatus && screen)
	{
		statusLoop=FALSE;
		statusPlugin=FALSE;
		pthread_join(readPositionThread,NULL);
	}
	
	return TRUE;
}

static Bool 
smackpadInitScreen (CompPlugin *p, 
					CompScreen *s)
{	
	screen=s;
	
	return TRUE;
}

static Bool 
smackpadInitDisplay (CompPlugin *p,
					 CompDisplay *d)
{	
	smackpadSetSensitivityRateNotify(d,smackpadUpdateSensitivityRate);
	smackpadSetInitiateKeyInitiate(d, smackpadInitiate);
	
	return TRUE;
}

static CompBool
smackpadInitObject (CompPlugin *p,
					CompObject *o)
{
	static InitPluginObjectProc dispTab[] = {
		(InitPluginObjectProc) 0, /* InitCore */
		(InitPluginObjectProc) smackpadInitDisplay,
		(InitPluginObjectProc) smackpadInitScreen,
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
smackpadFini (CompPlugin *p)
{
	if(readPositionThreadStatus==0)
	{
		statusLoop=FALSE;
		pthread_join(readPositionThread,NULL);
	}
}


CompPluginVTable smackpadVTable = {
	"smackpad",
	0,
	0,
	smackpadFini,
	smackpadInitObject,
	0,
	0,
	0
};

CompPluginVTable * 
getCompPluginInfo (void)
{
	return &smackpadVTable;
}
