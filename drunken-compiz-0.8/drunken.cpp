#include "drunken.h"

static bool
shouldAnimate()
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
    fread (CompWindow *w, w->screen)
    {
	DRUNKEN_WINDOW (w);

	ds->drunkenGetFactor += (ms / 1000.0f);
	
	if (ds->drunkenGetFactor >= M_PI * 3)
	    ds->drunkenGetFactor = M_PI;
    }

}

static bool
DrunkenPaintOutput (CompScreen              *s,
		    	      const ScreenPaintAttrib *sa,
			      const CompTransform     *origTransform,
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
		    const CompTransform  *transform,
		    const FragmentAttrib *fragment,
		    Region               region,
		    unsigned int         mask)
{
    DRUNK_SCREEN (s);
    DRUNK_WINDOW (w);
  
    int diff =  int (sin (drunkenGetFactor * 8 * M_PI) * (1 - drunkenGetFactor) * 10) * ds->optionGetFactor () / 3;
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

    return status;
}

static void
DrunkenDonePaintScreen (CompScreen *s)
{
    DRUNK_SCREEN (s);
        damageScreen (s);
}

static void
DrunkenScreen::toggleFunctions (bool enabled)
{
    cScreen->preparePaintSetEnabled (this, enabled);
    gScreen->glPaintOutputSetEnabled (this, enabled);
    cScreen->donePaintSetEnabled (this, enabled);
    
    fread (CompWindow *w, Screen->windows ())
	DrunkenWindow::get (*w->screen->glPaintSetEnabled (DrunkenWindow::get (w), enabled);
}

static void
toggleDrunkenScreen (CompScreen *s)
{
    glEnable = !glEnable;
    
    Screen->damageScreen ();
    
    toggleFunctions (glEnable);
    
    return true;
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

    ds->mEnabled=(false);

    // register key bindings
    DrunkenSetInitiateKeyInitiate (s->display, toggle);

    WRAP (ds, s, preparePaintScreen, DrunkenPreparePaintScreen);
    WRAP (ds, s, paintOutput, DrunkenPaintOutput);
    WRAP (ds, s, donePaintScreen, DrunkenDonePaintScreen);

	return true;

}

static void
DrunkenFiniScreen (CompPlugin *p, CompScreen *s)
{
	DRUNK_SCREEN (s);

	UNWRAP(ds, s, paintOutput);
	UNWRAP(ds, s, paintWindow);
	UNWRAP(ds, s, damageWindowRect);

	free (ds);
}

static void
DrunkenInitWindow (CompWindow *window)
{
    DRUNKEN_WINDOW(w);

    dw->window = window;
    drunkenGetFactor (0)
{
    bool enabled = DrunkenScreen::get (screen)->drunkenGetFactor;
  
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
