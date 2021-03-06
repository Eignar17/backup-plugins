/*
 * True shadows for Compiz-Fusion
 * 
 * Copyright 2008 Kevin Lange <klange@ogunderground.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <compiz-core.h>

#include <math.h>
#include <stdio.h>

#include <X11/extensions/shape.h>
#include "shadows_options.h"


#define PI 3.14159265358979323846
#define ADEG2RAD(DEG) ((DEG)*((PI)/(180.0)))

// Macros/*{{{*/
#define GET_SHADOWS_DISPLAY(d)                                       \
    ((SHDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define SHADOWS_DISPLAY(d)                      \
    SHDisplay *shd = GET_SHADOWS_DISPLAY (d)

#define GET_SHADOWS_SCREEN(s, shd)                                        \
    ((SHScreen *) (s)->base.privates[(shd)->screenPrivateIndex].ptr)

#define SHADOWS_SCREEN(s)                                                      \
    SHScreen *shs = GET_SHADOWS_SCREEN (s, GET_SHADOWS_DISPLAY (s->display))

#define GET_SHADOWS_WINDOW(w, shs)                                        \
    ((SHWindow *) (w)->base.privates[(shs)->windowPrivateIndex].ptr)

#define SHADOWS_WINDOW(w)                                         \
    SHWindow *shw = GET_SHADOWS_WINDOW  (w,                    \
                       GET_SHADOWS_SCREEN  (w->screen,            \
                       GET_SHADOWS_DISPLAY (w->screen->display)))


#define WIN_REAL_X(w) (w->attrib.x - w->input.left)
#define WIN_REAL_Y(w) (w->attrib.y - w->input.top)

#define WIN_REAL_W(w) (w->width + w->input.left + w->input.right)
#define WIN_REAL_H(w) (w->height + w->input.top + w->input.bottom)

/*}}}*/

typedef struct _SHDisplay{
    int screenPrivateIndex;

    HandleEventProc handleEvent;

    CompWindow *grabWindow;
    CompWindow *focusWindow;

} SHDisplay;

typedef struct _WTHead {
	float x;
	float y;
	float z;
} WTHead;

typedef struct _SHScreen {
    PreparePaintScreenProc	 preparePaintScreen;
    PaintOutputProc		 paintOutput;
    PaintWindowProc 		 paintWindow;
    CompTimeoutHandle mouseIntervalTimeoutHandle;
    int mouseX;
    int mouseY;
    int grabIndex;
    int rotatedWindows;
    int windowPrivateIndex;
    Bool trackMouse;
    
    DamageWindowRectProc damageWindowRect;
    WTHead head;
} SHScreen;

typedef struct _SHWindow{
    float depth;
    float manualDepth;
    float zDepth;
    float oldDepth;
    float newDepth;
    int timeRemaining; // 100 to 0
    int zIndex;
    Bool isAnimating;
    Bool isManualDepth;
    Bool grabbed;
} SHWindow;

int displayPrivateIndex;
static CompMetadata shadowsMetadata;

static void WTHandleEvent(CompDisplay *d, XEvent *ev){

    SHADOWS_DISPLAY(d);
    UNWRAP(shd, d, handleEvent);
    (*d->handleEvent)(d, ev);
    WRAP(shd, d, handleEvent, WTHandleEvent);
    
}

static Bool
windowIs3D (CompWindow *w)
{
    // Is this window one we need to consider
    // for shadow depth level?
    
    if (w->attrib.override_redirect)
	return FALSE;

    if (!(w->shaded || w->attrib.map_state == IsViewable))
	return FALSE;

    if (w->state & (CompWindowStateSkipPagerMask |
		    CompWindowStateSkipTaskbarMask))
	return FALSE;
	
    if (w->state & (CompWindowStateStickyMask))
    	return FALSE;

    if (w->type & (NO_FOCUS_MASK))
    	return FALSE;
    return TRUE;
}

static void WTPreparePaintScreen (CompScreen *s, int msSinceLastPaint) {

    CompWindow *w;
    SHADOWS_SCREEN (s);
    int maxDepth = 0;
    
    // First establish our maximum depth
    for (w = s->windows; w; w = w->next)
    {
	    SHADOWS_WINDOW (w);
	    if (!(WIN_REAL_X(w) + WIN_REAL_W(w) <= 0.0 || WIN_REAL_X(w) >= w->screen->width)) {
	        if (!(shw->isManualDepth)) {
	            if (!windowIs3D (w))
		            continue;
	            maxDepth++;
	        }
	    }
    }
    
    // Then set our windows as such
    for (w = s->windows; w; w = w->next)
    {
	    SHADOWS_WINDOW (w);
	    if (!(shw->isManualDepth) && shw->zDepth > 0.0) {
	        shw->zDepth = 0.0;
	    }
	    if (!(WIN_REAL_X(w) + WIN_REAL_W(w) <= 0.0 || WIN_REAL_X(w) >= w->screen->width)) {
	        if (!(shw->isManualDepth)) {
	            if (!windowIs3D (w))
		            continue;
	            maxDepth--;
	            float tempDepth = 0.0f - ((float)maxDepth * 0.01);
	            if (!shw->isAnimating) {
	                shw->newDepth = tempDepth;
	                if (shw->zDepth != shw->newDepth) {
	                    shw->oldDepth = shw->zDepth;
	                    shw->isAnimating = TRUE;
	                    shw->timeRemaining = 0;
	                }
	            } else {
	                if (shw->newDepth != tempDepth) {
	                    shw->newDepth = tempDepth;
	                    shw->oldDepth = shw->zDepth;
	                    shw->isAnimating = TRUE;
	                    shw->timeRemaining = 0;
	                } else {
                        shw->timeRemaining++;
                        float dz = (float)(shw->oldDepth - shw->newDepth) * (float)shw->timeRemaining / shadowsGetFadeTime (s);
                        shw->zDepth = shw->oldDepth - dz;
                        if (shw->timeRemaining >= shadowsGetFadeTime (s)) {
                            shw->isAnimating = FALSE;
                            shw->zDepth = shw->newDepth;
                            shw->oldDepth = shw->newDepth;
                        }
                    }
	            }
	        } else {
		        shw->zDepth = shw->depth;
	        }
	    } else {
	        shw->zDepth = 0.0f;
	    }
    }
    
    UNWRAP (shs, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, msSinceLastPaint);
    WRAP (shs, s, preparePaintScreen, WTPreparePaintScreen);
}


static Bool shouldPaintStacked(CompWindow *w) {
    
    // Should we draw the windows or not?
    // TODO: I may want to check for something,
    // so leave this here for now.
   
    return TRUE;
}


static Bool WTPaintWindow(CompWindow *w, const WindowPaintAttrib *attrib, 
	const CompTransform *transform, Region region, unsigned int mask){
	
	// Draw the window with its correct z level
    CompTransform wTransform = *transform;
    Bool status;
    
    SHADOWS_SCREEN(w->screen);
    SHADOWS_WINDOW(w);
    
     /* if (shouldPaintStacked(w)) {
        if (!(w->type == CompWindowTypeDesktopMask)) {
            if (!(WIN_REAL_X(w) + WIN_REAL_W(w) <= 0.0 || WIN_REAL_X(w) >= w->screen->width))
        	    matrixTranslate(&wTransform, 0.0, 0.0, shw->zDepth);
        	if (shw->zDepth != 0.0)
        	    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
        } else {
        	matrixTranslate(&wTransform, 0.0, 0.0, 0.0);
        }
    }  */
    
    
    
    damageScreen(w->screen);
    
    UNWRAP(shs, w->screen, paintWindow);
    status = (*w->screen->paintWindow)(w, attrib, &wTransform, region, mask);
    WRAP(shs, w->screen, paintWindow, WTPaintWindow);

    return status;
}


static Bool WTPaintOutput(CompScreen *s, const ScreenPaintAttrib *sAttrib, 
	const CompTransform *transform, Region region, CompOutput *output, unsigned int mask){
	
	Bool status;
	SHADOWS_SCREEN(s);
	CompTransform zTransform = *transform;
	//mask |= PAINT_SCREEN_CLEAR_MASK;
	
	UNWRAP (shs, s, paintOutput);
	status = (*s->paintOutput) (s, sAttrib, &zTransform, region, output, mask);
	WRAP (shs, s, paintOutput, WTPaintOutput);
	return status;
}

// Fairly standard stuff, I don't even mess with anything here...
static Bool WTDamageWindowRect(CompWindow *w, Bool initial, BoxPtr rect){

    Bool status = TRUE;
    SHADOWS_SCREEN(w->screen);
    if (!initial) {
	REGION region;
	region.rects = &region.extents;
	region.numRects = region.size = 1;
	region.extents.x1 = w->serverX;
	region.extents.y1 = w->serverY;
	region.extents.x2 = w->serverX + w->serverWidth;
	region.extents.y2 = w->serverY + w->serverHeight;
	damageScreenRegion (w->screen, &region);
	return TRUE;
    }
    UNWRAP(shs, w->screen, damageWindowRect);
    status = (*w->screen->damageWindowRect)(w, initial, rect);
    WRAP(shs, w->screen, damageWindowRect, WTDamageWindowRect);
    damagePendingOnScreen (w->screen);
    if (initial) {
    	damagePendingOnScreen(w->screen);
    }
    return status;
}

static Bool shadowsInitWindow(CompPlugin *p, CompWindow *w){
    SHWindow *shw;
    SHADOWS_SCREEN(w->screen);

    if( !(shw = (SHWindow*)malloc( sizeof(SHWindow) )) )
	return FALSE;

    shw->depth = 0.0; // Reset window actual depth
    shw->zDepth = 0.0; // Reset window z orders
    shw->isManualDepth = FALSE; // Reset windows to automatic
    shw->manualDepth = 0.0;
    shw->grabbed = 0;
    
    w->base.privates[shs->windowPrivateIndex].ptr = shw;
    
    return TRUE;
}

static void shadowsFiniWindow(CompPlugin *p, CompWindow *w){

    SHADOWS_WINDOW(w);
    SHADOWS_DISPLAY(w->screen->display);
    
    shw->depth = 0.0;
    

    if(shd->grabWindow == w){
	shd->grabWindow = NULL;
    }
    
    free(shw); 
}
/*}}}*/

// Screen init / clean/*{{{*/
static Bool shadowsInitScreen(CompPlugin *p, CompScreen *s){
    SHScreen *shs;

    SHADOWS_DISPLAY(s->display);

    if( !(shs = (SHScreen*)malloc( sizeof(SHScreen) )) )
	return FALSE;

    if( (shs->windowPrivateIndex = allocateWindowPrivateIndex(s)) < 0){
	free(shs);
	return FALSE;
    }
    

    s->base.privates[shd->screenPrivateIndex].ptr = shs;
    WRAP(shs, s, preparePaintScreen, WTPreparePaintScreen);
    WRAP(shs, s, paintWindow, WTPaintWindow);
    WRAP(shs, s, paintOutput, WTPaintOutput);

    WRAP(shs, s, damageWindowRect, WTDamageWindowRect);

    return TRUE;
}

static void shadowsFiniScreen(CompPlugin *p, CompScreen *s){

    SHADOWS_SCREEN(s);

    freeWindowPrivateIndex(s, shs->windowPrivateIndex);

    UNWRAP(shs, s, preparePaintScreen);
    UNWRAP(shs, s, paintWindow);
    UNWRAP(shs, s, paintOutput);
    

    UNWRAP(shs, s, damageWindowRect);

    free(shs);
}
/*}}}*/

// Display init / clean/*{{{*/
static Bool shadowsInitDisplay(CompPlugin *p, CompDisplay *d){

    SHDisplay *shd; 

    if( !(shd = (SHDisplay*)malloc( sizeof(SHDisplay) )) )
	return FALSE;
    
    // Set variables correctly
    shd->grabWindow = 0;
    shd->focusWindow = 0;
    
     
    if( (shd->screenPrivateIndex = allocateScreenPrivateIndex(d)) < 0 ){
	free(shd);
	return FALSE;
    }
    
    
    d->base.privates[displayPrivateIndex].ptr = shd;
    WRAP(shd, d, handleEvent, WTHandleEvent);
    
    return TRUE;
}

static void shadowsFiniDisplay(CompPlugin *p, CompDisplay *d){

    SHADOWS_DISPLAY(d);
    
    freeScreenPrivateIndex(d, shd->screenPrivateIndex);

    UNWRAP(shd, d, handleEvent);

    free(shd);
}
/*}}}*/

// Object init / clean
static CompBool shadowsInitObject(CompPlugin *p, CompObject *o){

    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, // InitCore
	(InitPluginObjectProc) shadowsInitDisplay,
	(InitPluginObjectProc) shadowsInitScreen,
	(InitPluginObjectProc) shadowsInitWindow
    };

    RETURN_DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), TRUE, (p, o));
}

static void shadowsFiniObject(CompPlugin *p, CompObject *o){

    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, // FiniCore
	(FiniPluginObjectProc) shadowsFiniDisplay,
	(FiniPluginObjectProc) shadowsFiniScreen,
	(FiniPluginObjectProc) shadowsFiniWindow
    };

    DISPATCH(o, dispTab, ARRAY_SIZE(dispTab), (p, o));
}

// Plugin init / clean
static Bool shadowsInit(CompPlugin *p){
    if( (displayPrivateIndex = allocateDisplayPrivateIndex()) < 0 )
	return FALSE;

	compAddMetadataFromFile (&shadowsMetadata, p->vTable->name);

    return TRUE;
}

static void shadowsFini(CompPlugin *p){
    if(displayPrivateIndex >= 0)
	freeDisplayPrivateIndex( displayPrivateIndex );
	
}

// Plugin implementation export
CompPluginVTable shadowsVTable = {
    "shadows",
    0,
    shadowsInit,
    shadowsFini,
    shadowsInitObject,
    shadowsFiniObject,
    0,
    0
};

CompPluginVTable *getCompPluginInfo (void){ return &shadowsVTable; }

