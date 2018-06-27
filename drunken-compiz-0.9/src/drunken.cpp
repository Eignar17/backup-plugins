#include "drunken.h"

COMPIZ_PLUGIN_20090315 (drunken, DrunkenPluginVTable)

bool
DrunkenWindow::shouldAnimate ()
{
    /* Override Redirect windows are painful */
    if (window->overrideRedirect ())
	return false;
    
    /* Don't do this for panels docks or desktops */
    if (window->wmType () & (CompWindowTypeDockMask | CompWindowTypeDesktopMask))
	return false;
    
    /* Don't do this for invisible windows */
    if (!window->mapNum () || !window->isViewable ())
	return false;
    
    return true;
}

void
DrunkenScreen::preparePaint (int ms)
{
    foreach (CompWindow *w, screen->windows ())
    {
	DRUNK_WINDOW (w);

	dw->mDrunkFactor += (ms / 1000.0f);
	
	if (dw->mDrunkFactor >= M_PI * 3)
	    dw->mDrunkFactor = M_PI;
    }
}

bool
DrunkenScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			      const GLMatrix	       &transform,
			      const CompRegion	       &region,
			      CompOutput	       *output,
			      unsigned int	       mask)
{
    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    
    return gScreen->glPaintOutput (attrib, transform, region, output, mask);
}

bool
DrunkenWindow::glPaint (const GLWindowPaintAttrib &attrib,
			const GLMatrix		  &transform,
			const CompRegion	  &region,
			unsigned int		  mask)
{
    DRUNK_SCREEN (screen);
  
    int diff =  int (sin (mDrunkFactor * 8 * M_PI) * (1 - mDrunkFactor) * 10) * ds->optionGetFactor () / 3;
    bool status;
    
    GLMatrix wTransform1 (transform);
    GLMatrix wTransform2 (transform);
    GLWindowPaintAttrib wAttrib (attrib);

    wAttrib.opacity *= 0.5;
    wTransform1.translate (-diff, 0.0f, 0.0f);
    
    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
    
    status = gWindow->glPaint (wAttrib, wTransform1, region, mask);
    
    wTransform2.translate (diff, 0.0f, 0.0f);
    
    status |= gWindow->glPaint (wAttrib, wTransform2, region, mask);

    return status;
}

void
DrunkenScreen::donePaint ()
{
    cScreen->damageScreen ();
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

bool
DrunkenScreen::toggle ()
{
    mEnabled = !mEnabled;
    
    cScreen->damageScreen ();
    
    toggleFunctions (mEnabled);
    
    return true;
}

DrunkenScreen::DrunkenScreen (CompScreen *screen) :
    PluginClassHandler <DrunkenScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mEnabled (false)
{
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);
    
    optionSetInitiateKeyInitiate (boost::bind (&DrunkenScreen::toggle, this));
}

DrunkenWindow::DrunkenWindow (CompWindow *window) :
    PluginClassHandler <DrunkenWindow, CompWindow> (window),
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window)),
    mDrunkFactor (0)
{
    bool enabled = DrunkenScreen::get (screen)->mEnabled;
  
    CompositeWindowInterface::setHandler (cWindow, enabled);
    GLWindowInterface::setHandler (gWindow, enabled);
}

bool
DrunkenPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;
    
    return true;
}