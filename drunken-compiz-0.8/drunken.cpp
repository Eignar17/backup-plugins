#include "drunken.h"

static Bool
shouldAnimate(CompWindow *window)
{
    /* Override Redirect windows are painful */
    if (window->attrib.override_redirect)
	return FALSE;
    
    /* Don't do this for panels docks or desktops */
   if (window->wmType & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return FALSE;
    
    /* Don't do this for invisible windows */
    if (window->mapNum || window->attrib.map_state == IsViewable)
	return FALSE;
    
    return TRUE;
}

static void
DrunkenPreparePaintScreen (CompScreen *s,
			   int ms)
{
    CompWindow *w;

    for (w = s->windows; w; w = w->next)
    {
        DRUNK_WINDOW (w);

        dw->mDrunkFactor += (ms / 1000.0f);

        if (dw->mDrunkFactor >= M_PI * 3)
            dw->mDrunkFactor = M_PI;
    }
}

static Bool
DrunkenPaintOutput (CompScreen              *s,
		     const ScreenPaintAttrib *attrib,
		     const CompTransform     *transform,
		     Region                  region,
		     CompOutput              *output,
		     unsigned int            mask)
{
    DRUNK_SCREEN (s);

    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    UNWRAP (ds, s, paintOutput);
    Bool status = (*s->paintOutput) (s, attrib, transform, region, output, mask);
    WRAP (ds, s, paintOutput, DrunkenPaintOutput);

    return status;
}

static bool
DrunkenPaintWindow (CompWindow           *w,
                    const CompTransform   *transform,
                    const WindowPaintAttrib *attrib,    	      
                    Region                region,
                    unsigned int          mask)
{
    CompTransform wTransform1 = *transform;
    CompTransform wTransform2 = *transform;
    WindowPaintAttrib wAttrib = *attrib;

    DRUNK_SCREEN (w->screen);
    DRUNK_WINDOW (w);
  
    int diff =  int (sin (mDrunkFactor * 8 * M_PI) * (1 - mDrunkFactor) * 10) * ds->optionGetFactor () / 3;
    bool status;

    wAttrib.opacity *= 0.5;
    matrixTranslate (&wTransform1, -diff, 0.0f, 0.0f);

    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

    UNWRAP(ds, w->screen, paintWindow);
    status = (*w->screen->paintWindow) (w, &wTransform1, &wAttrib, region, mask);
    WRAP(ds, w->screen, paintWindow, DrunkenPaintPaintWindow);

    matrixTranslate (&wTransform2, diff, 0.0f, 0.0f);

    UNWRAP(ds, w->screen, paintWindow);
    status |= (*w->screen->paintWindow) (w, &wTransform2, &wAttrib, region, mask);
    WRAP(ds, w->screen, paintWindow, DrunkenPaintPaintWindow);

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

static void
Toggle (CompScreen *s,
	CompWindow *window)
{
    DrunkenScreen *ds = (DrunkenScreen*)s->base.privates[display->screenPrivateIndex].ptr;

    for (unsigned int i = 0; i < screen->window ().size (); i++)
    DrunkenWindow (s->window ()[i])->window->s->paintWindow (DrunkenWindow (s->window ()[i])->window, enabled);

    ds->mEnabled = !ds->mEnabled;

        damageScreen (s);
    }

    return true;
}


static Bool
DrunkenInitScreen (CompPlugin *p, CompScreen *s)
{
    DrunkenScreen *ds;

    DRUNK_DISPLAY (s->display);

    ds = calloc (1, sizeof (DrunkenScreen));

    if (!ds)
        return FALSE;

    s->base.privates[dd->screenPrivateIndex].ptr = ds;

    ds->mEnabled = FALSE;

    // register key bindings
    DrunkenSetInitiateKeyInitiate (s->display, toggle);

    WRAP (ds, s, preparePaintScreen, DrunkenPreparePaintScreen);
    WRAP (ds, s, paintOutput, DrunkenPaintOutput);
    WRAP (ds, s, donePaintScreen, DrunkenDonePaintScreen);

    return TRUE;
}

static void
DrunkenFiniScreen (CompPlugin *p,
		   CompScreen *s)
{
	DRUNK_SCREEN (s);

    UNWRAP (ds, s, preparePaintScreen);
    UNWRAP (ds, s, paintOutput);
    UNWRAP (ds, s, donePaintScreen);

	free (ds);
}

static void
DrunkenInitWindow (CompPlugin *p, CompWindow *w)
{
    DrunkenWindow *dw;
    DRUNK_WINDOW(w->screen);

    sow = (DrunkenWindow*)calloc (1, sizeof (DrunkenWindow));
    if (!dw)
        return FALSE;

    dw->mDrunkFactor = 0;

    bool enabled = GET_DRUNK_SCREEN (w->screen, GET_DRUNK_DISPLAY (w->screen->display))->mEnabled;

    w->base.privates[ds->windowPrivateIndex].ptr = dw;

    return TRUE;
}

static void
DrunkeFiniWindow (CompPlugin *p, CompWindow *w)
{
    DRUNK_WINDOW(w);

    free(dw);
}

static Bool
DrunkenInitDisplay (CompPlugin  *p,
		     CompDisplay *d)
{
    int index;
    DrunkenDisplay *dd;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
        return FALSE;

    dd = calloc (1, sizeof (DrunkenDisplay) );

    if (!dd)
	return FALSE;

    dd->screenPrivateIndex = allocateScreenPrivateIndex (d);

    if (dd->screenPrivateIndex < 0)
    {
	free (dd);
	return FALSE;
    }

    d->base.privates[displayPrivateIndex].ptr = ds;

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
	return FALSE;

    return TRUE;
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
