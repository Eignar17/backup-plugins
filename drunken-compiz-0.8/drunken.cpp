#include "drunken.h"

bool
DrunkenWindow::shouldAnimate ()
{
    /* Override Redirect windows are painful */
    if (w->attrib.override_redirect)
	return false;
    
    /* Don't do this for panels docks or desktops */
   if (!(w->wmType & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return false;
    
    /* Don't do this for invisible windows */
    if (w->mapNum || !w->attrib.map_state == IsViewable)
	return false;
    
    return true;
}

static void
DrunkenPreparePaintScreen (CompScreen *s,
			    int        ms)
{
    fread (CompWindow *w, w->screen)
    {
	DRUNK_WINDOW (w);

	dw->drunkenGetFactor += (ms / 1000.0f);
	
	if (dw->drunkenGetFactor >= M_PI * 3)
	    dw->drunkenGetFactor = M_PI;
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
    DRUNK_SCREEN (s);
    DRUNK_WINDOW (w);
  
    int diff =  int (sin (drunkenGetFactor * 8 * M_PI) * (1 - drunkenGetFactor) * 10) * ds->optionGetFactor () / 3;
    Bool status;
    
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

void
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

static Bool
DrunkenInitScreen (CompPlugin *p,
		    CompScreen *s)
{
	DrunkenInitScreen *s;

    DRUNK_DISPLAY (s->display);

    fs = calloc (1, sizeof (DrunkenInitScreen) );

    if (!fs)
	return FALSE;

    s->base.privates[vd->screenPrivateIndex].ptr = s;

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
    DRUNK_WINDOW(w);

    pow->window = window;
    drunkenGetFactor (0)
{
    bool enabled = DrunkenScreen::get (screen)->drunkenGetFactor;
  
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
        (InitPluginObjectProc) DrunkenInitWindow
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
