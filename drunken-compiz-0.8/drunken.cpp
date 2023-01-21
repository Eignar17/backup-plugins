#include "drunken.h"

static bool
shouldAnimate(CompWindow *window)
{
    /* Override Redirect windows are painful */
    if (w->attrib.override_redirect)
	return false;
    
    /* Don't do this for panels docks or desktops */
   if (w->wmType & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return false;
    
    /* Don't do this for invisible windows */
    if (w->mapNum || w->attrib.map_state == IsViewable)
	return false;
    
    return true;
}

static void
DrunkenPreparePaintScreen (CompScreen *s,
			    int        ms)
{
    DRUNK_SCREEN (s);

    UNWRAP (ds, s, preparePaintScreen);
    (*s->preparePaintScreen) (s, ms);
    WRAP (ds, s, preparePaintScreen, DrunkenPreparePaintScreen);

    fread (CompWindow *w, w->screen)
    {
	DRUNK_WINDOW (w);

	ds->drunkenGetFactor += (ms / 1000.0f);
	
	if (ds->drunkenGetFactor >= M_PI * 3)
	    ds->drunkenGetFactor = M_PI;
    }

}

static bool
DrunkenPaintOutput (CompScreen              *s,
		    	      const ScreenPaintAttrib *sa,
			      const CompTransform     *mTransform,
			      Region	               region,
			      CompOutput	       *output,
			      unsigned int	       mask)
{
    bool status;
    CompTransform *mTransform;
	
    DRUNK_SCREEN (s);
{
    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
}

    UNWRAP (ds, s, paintOutput);
    status = (*s->paintOutput) (s, sa, mTransform, region, output, mask);
    WRAP (ds, s, paintOutput, DrunkenPaintOutput);
     free (mTransform);

     return status;
}

static bool
DrunkenPaintWindow (CompWindow           *w,
		              const CompTransform	   *wTransform,
		    	      const WindowPaintAttrib *attrib,    	      
		    	      CompMatrix *matrix,
		    	      Region     region,
		    	      unsigned int		   mask)
{
    DRUNK_WINDOW (w);    
    DRUNK_SCREEN (w->screen);
  
    int diff = (int) (sin (drunkenGetFactor * 8 * M_PI) * (1 - drunkenGetFactor) * 10 * optionGetFactor (ds->o)) / 3;
    bool status;

    CompMatrix wTransform1 (Transform);
    CompMatrix wTransform2 (transform);
    WindowPaintAttrib *mAttrib;

    mAttrib.opacity *= 0.5;
    wTransform1.translate (-diff, 0.0f, 0.0f);

    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
    
    status = (*w->screen->paintWindow) (w, mAttrib, wTransform1, region, mask);
    
    wTransform2.translate (diff, 0.0f, 0.0f);
    
    status |= (*w->screen->paintWindow) (w, mAttrib, wTransform2, region, mask);

    UNWRAP (ds, w->screen, paintWindow);
    bool status = (*w->screen->paintWindow) (w, mAttrib, mTransform, region, mask);
    WRAP (ds, w->screen, paintWindow, DrunkenPaintWindow);

    return status;
}

static void
DrunkenDonePaintScreen (CompScreen *s)
{
    DRUNK_SCREEN (s);
        damageScreen (s);

    UNWRAP (ds, s, donePaintScreen);
    (*s->donePaintScreen) (s);
    WRAP (ds, s, donePaintScreen, DrunkenDonePaintScreen);
}

static void toggleDrunkenScreen (CompScreen *s)
{
    DRUNK_SCREEN (s);
    ds->enabled = !ds->enabled;
    if (ds->enabled)
{
    s->preparePaintScreen = DrunkenPreparePaintScreen;
    s->paintOutput = DrunkenPaintOutput;
    s->paintWindow = DrunkenPaintWindow;
    s->donePaintScreen = DrunkenDonePaintScreen;
}
    else
{
    s->preparePaintScreen = preparePaintScreen;
    s->paintOutput = paintOutput;
    s->paintWindow = paintWindow;
    s->donePaintScreen = donePaintScreen;
}

    damageScreen (s);

}

static void
toggleDrunkenScreen (CompScreen *s)
{
    DRUNK_SCREEN (s);
    ds->enabled = !ds->enabled;
    s->preparePaintScreen = (ds->enabled) ? DrunkenPreparePaintScreen : preparePaintScreen;
    s->paintOutput = (ds->enabled) ? DrunkenPaintOutput : paintOutput;
    s->paintWindow = (ds->enabled) ? DrunkenPaintWindow : paintWindow;
    s->donePaintScreen = (ds->enabled) ? DrunkenDonePaintScreen : donePaintScreen;
    damageScreen (s);
}

static bool
DrunkenInitScreen (CompPlugin *p,
		    CompScreen *s)
{
	DrunkenInitScreen *ds;

    DRUNK_DISPLAY (s->display);

    ds = calloc (1, sizeof (DrunkenScreen) );
    //ds = new DrunkenScreen;
    if (!ds)
	return false;

       ds->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ds->windowPrivateIndex < 0)
    {
        free (ds);
        return false;
    }

    ds->enabled=(false);

    // register key bindings
    DrunkenSetInitiateKeyInitiate (s->display, toggleFunctions);

    WRAP (ds, s, preparePaintScreen, DrunkenPreparePaintScreen);
    WRAP (ds, s, paintOutput, DrunkenPaintOutput);
    WRAP (ds, s, donePaintScreen, DrunkenDonePaintScreen);

    s->base.privates[dd->screenPrivateIndex].ptr = ds;

	return true;

}

static void
DrunkenFiniScreen (CompPlugin *p, CompScreen *s)
{
	DRUNK_SCREEN (s);

	freeWindowPrivateIndex (s, ds->windowPrivateIndex);
      
	UNWRAP (ds, s, preparePaintScreen);
	UNWRAP (ds, s, paintOutput);
	UNWRAP (ds, s, paintWindow);
        UNWRAP (ds, s, donePaintScreen);

	free (ds);
}

static void
DrunkenInitWindow (CompWindow *window)
{
    DRUNK_WINDOW(w);

dw->setWindow (window);
dw->drunkenGetFactor = 0.0f;

bool enabled = GET_DRUNK_SCREEN (w->screen, GET_DRUNK_DISPLAY (w->screen->display))->enabled;


static void
DrunkeFiniWindow (CompPlugin *p, CompWindow *w)
{
    DRUNK_WINDOW(w);

    free(dw);
}

static bool
DrunkenInitDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    int index;
    DrunkenDisplay *dd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
        return false;

    dd = calloc (1, sizeof (DrunkenDisplay) );

    if (!dd)
	return false;

    dd->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (dd->screenPrivateIndex < 0)
    {
	free (dd);
	return false;
    }

    d->base.privates[displayPrivateIndex].ptr = dd;

    DrunkenSetInitiateKeyInitiate (dd, DrunkenInitiate);
    DrunkenSetInitiateKeyTerminate (dd, DrunkenTerminate);

    return true;
}

static void
DrunkenFiniDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    DRUNK_DISPLAY (d);

    freeScreenPrivateIndex (d, dd->screenPrivateIndex);
    free (dd);
}

static bool
DrunkenInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();

    if (displayPrivateIndex < 0)
	return false;

    return true;
}

static bool
DrunkenInitObject (CompPlugin *p,
		    CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0,
	(InitPluginObjectProc) DrunkenInitDisplay,
	(InitPluginObjectProc) DrunkenInitScreen
        (InitPluginObjectProc) DrunkenInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), true, (p, o));
}

static void
DrunkenFiniObject (CompPlugin *p,
		    CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0,
	(FiniPluginObjectProc) DrunkenFiniDisplay,
	(FiniPluginObjectProc) DrunkenFiniScreen
	(FiniPluginObjectProc) DrunkenFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static void
DrunkenFini (CompPlugin *p)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}
	
CompPluginVTable DrunkenVTable = {
{
    "Drunken",
    0,
    DrunkenInit,
    DrunkenFini,
    DrunkenInitObject,
    DrunkenFiniObject,
    0,
    0,
};
 CompPluginVTable *
getCompPluginInfo (void)
{
    return &DrunkenVTable;
}
