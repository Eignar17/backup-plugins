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
DrunkenPreparePaintScreen (CompScreen *s, int ms)
{
    DRUNK_SCREEN (s);
    CompWindow *w;

    for (w = ds->windows; w; w = dw->next)
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
			      const CompTransform     *Transform,
			      Region	               region,
			      CompOutput	       *output,
			      unsigned int	       mask)
{
	
    DRUNK_SCREEN (s);
{
    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
}
    UNWRAP (ds, s, paintOutput);
    status = (*s->paintOutput) (s, sAttrib, transform, region, output, mask);
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

    DRUNK_SCREEN (w->screen);
    DRUNK_WINDOW (w);
  
    int diff =  int (sin (mDrunkFactor * 8 * M_PI) * (1 - mDrunkFactor) * 10) * ds->optionGetFactor () / 3;
    bool status;

    CompTransform wTransform1 = *transform;
    CompTransform wTransform2 = *transform;
    WindowPaintAttrib wAttrib = *attrib;

    wAttrib->opacity *= dw->0.5;
    matrixTranslate (&wTransform1, -diff, 0.0f, 0.0f);

    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

    UNWRAP(ds, w->screen, paintWindow);
    status = (*w->screen->paintWindow) (w, &wAttrib, &wTransform1, region, mask);
    WRAP(ds, w->screen, paintWindow, DrunkenPaintPaintWindow);

    matrixTranslate (&wTransform2, diff, 0.0f, 0.0f);

    UNWRAP(ds, w->screen, paintWindow);
    status |= (*w->screen->paintWindow) (w, &wAttrib, &wTransform2, region, mask);
    WRAP(ds, w->screen, paintWindow, DrunkenPaintPaintWindow);
    }
    {
	UNWRAP(ds, w->screen, paintWindow);
	status = (*w->screen->paintWindow) (w, attrib, transform, region, mask);
	WRAP(ds, w->screen, paintWindow, DrunkenPaintWindow);
    }

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
toggleFunctions (CompScreen *s)
{
    DRUNK_SCREEN (s);
    ds->mEnabled = !ds->mEnabled;
    if (ds->mEnabled)

    damageScreen (s);

    UNWRAP (sos, w->screen, toggleFunctions);
    (*w->screen->toggleFunctions) (w, toggleFunctions, mEnabled, mask);
     WRAP (sos, w->screen, toggleFunctions, DrunkentoggleFunctions);
}

static void
toggle (CompScreen *s)
{
    DRUNK_SCREEN (s);
    ds->mEnabled = !ds->mEnabled;

    WRAP (ds, s, preparePaintScreen, DrunkenPreparePaintScreen);
    WRAP (ds, s, paintOutput, DrunkenPaintOutput);
    WRAP (ds, s, DrunkenPaintWindow, paintWindow);
    WRAP (ds, s, donePaintScreen, DrunkenDonePaintScreen);
    damageScreen (s);
}

static bool
DrunkenScreen  (CompPlugin *p, CompWindow *w)
{
    DrunkenInitScreen *ds;
    DRUNK_DISPLAY (s->display);

    sow = (DrunkenWindow*)calloc (1, sizeof (DrunkenWindow));
    if (!dw)
        return FALSE;

       ds->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ds->windowPrivateIndex < 0)
    {
        free (ds);
        return FALSE;
    }

    ds->mEnabled=(FALSE);

    // register key bindings
    DrunkenSetInitiateKeyInitiate (s->display, toggleFunctions);

    WRAP (ds, s, preparePaintScreen, DrunkenPreparePaintScreen);
    WRAP (ds, s, paintOutput, DrunkenPaintOutput);
    WRAP (ds, s, DrunkenPaintWindow, paintWindow);
    WRAP (ds, s, paintTransformedOutput, DrunkenPaintTransformedOutput);
    WRAP (ds, s, donePaintScreen, DrunkenDonePaintScreen);


    s->base.privates[dd->screenPrivateIndex].ptr = ds;

	return TRUE;

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
DrunkenInitWindow (CompPlugin *p, CompWindow *w)
{
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

    DrunkenSetInitiateKeyInitiate (dd, DrunkenInitiate);

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
