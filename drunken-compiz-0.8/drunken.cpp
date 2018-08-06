#include "drunken.h"

bool
DrunkenWindow::shouldAnimate ()
{
    /* Override Redirect windows are painful */
    if (w->overrideRedirect ())
	return false;
    
    /* Don't do this for panels docks or desktops */
   if (!(w->type & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return false;
    
    /* Don't do this for invisible windows */
    if (w->mapNum () || !w->isViewable ())
	return false;
    
    return true;
}

static void
DrunkenPreparePaintScreen (CompScreen *s,
			    int        ms)
{
    foreach (CompWindow *w, screen->w ())
    {
	DRUNK_WINDOW (w);

	dw->mDrunkFactor += (ms / 1000.0f);
	
	if (dw->mDrunkFactor >= M_PI * 3)
	    dw->mDrunkFactor = M_PI;
    }

}

static Bool
DrunkenPaintOutput (CompScreen              *s,
		    	      const ScreenPaintAttrib *sa,
			      const CompTransform     *origTransform,
			      Region	               region,
			      CompOutput	       *output,
			      unsigned int	       mask)
{
    Bool status;
    CompTransform *mTransform;
	
    DRUNK_SCREEN (s);
{
    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
}

    UNWRAP (dw, s, paintOutput);
    status = (*s->paintOutput) (s, sa, mTransform, region, output, mask);
    WRAP (dw, s, paintOutput, DrunkenPaintOutput);
     free (mTransform);

     return status;
}

static Bool
DrunkenPaintWindow (CompWindow           *w,
		    const CompTransform  *transform,
		    const FragmentAttrib *fragment,
		    Region               region,
		    unsigned int         mask)
{
    DRUNK_SCREEN(w->screen);
    DRUNK_WINDOW(w);
  
    int diff =  int (sin (mDrunkFactor * 8 * M_PI) * (1 - mDrunkFactor) * 10) * ds->optionGetFactor () / 3;
    bool status;
    
    GLMatrix wTransform1 (Transform);
    GLMatrix wTransform2 (transform);
    WindowPaintAttrib *mAttrib;

    wAttrib.opacity *= 0.5;
    wTransform1.translate (-diff, 0.0f, 0.0f);
    
    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
    
   status = gWindow->glPaint (wAttrib, wTransform1, region, mask);
    
    wTransform2.translate (diff, 0.0f, 0.0f);
    
    status |= gWindow->glPaint (wAttrib, wTransform2, region, mask);

    return status;
}

static void
DrunkenDonePaintScreen (CompScreen *s)
{
    DRUNK_SCREEN (s);
        damageScreen (s);
}

void
DrunkenScreen::toggleFunctions (bool enabled)
{
    cScreen->preparePaintSetEnabled (this, enabled);
    gScreen->glPaintOutputSetEnabled (this, enabled);
    cScreen->donePaintSetEnabled (this, enabled);
    
    foreach (CompWindow *w, screen->windows ())
	DrunkenWindow::get (w)->gWindow->glPaintSetEnabled (DrunkenWindow::get (w), enabled);
}

static void
toggleDrunkenScreen (CompScreen *s)
{
    mEnabled = !mEnabled;
    
    cScreen->damageScreen ();
    
    toggleFunctions (mEnabled);
    
    return true;
}

static Bool
DrunkenInitScreen (CompPlugin *p,
		    CompScreen *s)
{
	DrunkenScreen *vs;

    DRUNK_DISPLAY (s->display);

    fs = calloc (1, sizeof (DrunkenScreen) );

    if (!fs)
	return FALSE;

    s->base.privates[vd->screenPrivateIndex].ptr = vs;

	WRAP (dw, s, preparePaintScreen, DrunkenPreparePaintScreen);
	WRAP (dw, s, donePaintScreen, DrunkenDonePaintScreen);
	WRAP (dw, s, paintScreen, DrunkenPaintScreen);

	return TRUE;

}

static void
DrunkenFiniScreen (CompPlugin *p, CompScreen *s)
{
	DRUNK_SCREEN (s);

	UNWRAP(dw, s, paintOutput);
	UNWRAP(dw, s, paintWindow);
	UNWRAP(dw, s, damageWindowRect);

	free (dw);
}

static void
DrunkenInitWindow (CompWindow *window)
{
    DRUNK_WINDOW(window);

    sow->window = window;
    mDrunkFactor (0)
{
    bool enabled = DrunkenScreen::get (screen)->mEnabled;
  
}

static Bool
DrunkenInitDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    int index;
    DrunkenDisplay *dwd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
        return FALSE;

    fd = calloc (1, sizeof (DrunkenDisplay) );

    if (!fd)
	return FALSE;

    sod->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (sod->screenPrivateIndex < 0)
    {
	free (sod);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = sod;

    DrunkenSetInitiateKeyInitiate (d, DrunkenInitiate);
    DrunkenSetInitiateKeyTerminate (d, DrunkenTerminate);

    return TRUE;
}

static void
DrunkenFiniDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    DRUNK_DISPLAY (d);

    freeScreenPrivateIndex (d, sod->screenPrivateIndex);
    free (sod);
}

static Bool
DrunkenInit (CompPlugin *p)
{
    displayPrivateIndex = allocateDisplayPrivateIndex ();

    if (displayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static CompBool
DrunkenInitObject (CompPlugin *p,
		    CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0,
	(InitPluginObjectProc) DrunkenInitDisplay,
	(InitPluginObjectProc) DrunkenInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
DrunkenFiniObject (CompPlugin *p,
		    CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0,
	(FiniPluginObjectProc) DrunkenFiniDisplay,
	(FiniPluginObjectProc) DrunkenFiniScreen
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
    return &stereo3dVTable;
