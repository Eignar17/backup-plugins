#include "hoverfx.h"
#include <cmath>

COMPIZ_PLUGIN_20090315 (hoverfx, HoverPluginVTable);

void
HoverWindow::toggleFunctions (bool enabled)
{
    gWindow->glPaintSetEnabled (this, enabled);
    cWindow->damageRectSetEnabled (this, enabled);
    
    if (HoverScreen::get (screen)->mHoveredWindows.size ())
	HoverScreen::get (screen)->toggleFunctions (true);
}

void
HoverScreen::toggleFunctions (bool enabled)
{
    cScreen->preparePaintSetEnabled (this, enabled);
    gScreen->glPaintOutputSetEnabled (this, enabled);
    cScreen->donePaintSetEnabled (this, enabled);
}

/* Returns true if it makes sense for this window to be animated on hover */
bool
HoverWindow::shouldAnimate ()
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
    
    

/* Handle X11 Events, on EnterNotify set the hovered bit,
 * and unset on LeaveNotify
 */

void
HoverScreen::handleEvent (XEvent *event)
{
    switch (event->type)
    {
	case ButtonPress:
	{
	    Window xid = event->xbutton.window;
	    
	    CompWindow *w = screen->findWindow (xid);
	    
	    if (w && HoverWindow::get (w)->shouldAnimate () && HoverWindow::get (w)->mIsHovered)
	    {
		HOVER_WINDOW (w);
		
		if (optionGetEffectType () == HoverfxOptions::EffectTypeScale)
		{
		    hw->mOld = hw->mCurrent;
		    hw->mTarget = 1.0f;
		}
		else if (optionGetEffectType () == HoverfxOptions::EffectTypeOpacity ||
		    optionGetEffectType () == HoverfxOptions::EffectTypeBrightness ||
		    optionGetEffectType () == HoverfxOptions::EffectTypeSaturation)
		{
		    hw->mOld = hw->mCurrent;
		    hw->mTarget = 100.0f;
		}
		hw->mAnimate = true;
		hw->toggleFunctions (true);
		hw->cWindow->addDamage ();
	    }
	}
	case EnterNotify:
	{
	    Window xid = event->xcrossing.window;
	    
	    CompWindow *w = screen->findWindow (xid);
	    
	    if (w && HoverWindow::get (w)->shouldAnimate () && !HoverWindow::get (w)->mIsHovered)
	    {
		HOVER_WINDOW (w);

		hw->mAnimate = true;
		if (optionGetEffectType () == HoverfxOptions::EffectTypeScale)
		{
		    hw->mDefault = 1.0f;
		    hw->mOld = hw->mCurrent;
		    hw->mTarget = optionGetScaleFactor ();
		}
		else if (optionGetEffectType () == HoverfxOptions::EffectTypeOpacity)
		{
		    hw->mDefault = 1.0f;
		    hw->mOld = hw->mCurrent;
		    hw->mTarget = optionGetOpacity ();
		}
		else if (optionGetEffectType () == HoverfxOptions::EffectTypeBrightness)
		{
		    hw->mDefault = 1.0f;
		    hw->mOld = hw->mCurrent;
		    hw->mTarget = optionGetBrightness ();
		}
		else if (optionGetEffectType () == HoverfxOptions::EffectTypeSaturation)
		{
		    hw->mDefault = 1.0f;
		    hw->mOld = hw->mCurrent;
		    hw->mTarget = optionGetSaturation ();
		}
		hw->mIsHovered = true;
		mHoveredWindows.push_back (w);
		hw->toggleFunctions (true);
	    }
		
	}
	break;
	case LeaveNotify:
	{
	    Window xid = event->xcrossing.window;
	    
	    CompWindow *w= screen->findWindow (xid);
	    
	    if (w && HoverWindow::get (w)->shouldAnimate ())
	    {
		HOVER_WINDOW (w);
		
		hw->mAnimate = true;
		if (optionGetEffectType () == HoverfxOptions::EffectTypeScale)
		{
		    hw->mOld = hw->mCurrent;
		    hw->mTarget = 1.0f;
		}
		else if (optionGetEffectType () == HoverfxOptions::EffectTypeOpacity ||
			 optionGetEffectType () == HoverfxOptions::EffectTypeBrightness ||
			 optionGetEffectType () == HoverfxOptions::EffectTypeSaturation)
		{
		    hw->mOld = hw->mCurrent;
		    hw->mTarget = 100.0f;
		}
		hw->toggleFunctions (true);
		hw->mIsHovered = false;
		
	    }
	}
	break;
	default:
	    break;
    }
    
    screen->handleEvent (event);
}

/* Before painting the screen, determine how much time has lapsed and then
 * increment animation time on windows
 */

void
HoverScreen::preparePaint (int ms)
{
    CompWindowList::iterator it;
  
    for (it = mHoveredWindows.begin (); it != mHoveredWindows.end (); it++)
    {
	CompWindow *w = *it;
      
	HOVER_WINDOW (w);
	
	if (hw->mAnimate)
	{
	    float 	steps = (float) ms / (float) optionGetTime ();
	    
	    if (steps < 0.05)
		steps = 0.05;
	    
	    hw->mCurrent += steps * (hw->mTarget - hw->mCurrent);
	    if (fabsf (hw->mTarget - hw->mCurrent) < 0.005)
	    {
		hw->mAnimate = false;
		hw->mCurrent = hw->mTarget;
	    }
	    if (hw->mCurrent == hw->mDefault)
	    {
		hw->mAnimate = false;
		mHoveredWindows.remove (w);
		it = mHoveredWindows.begin ();
		hw->toggleFunctions (false);
	    }
	}
    }

    cScreen->preparePaint (ms);
}

bool
HoverScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &region,
			    CompOutput		      *output,
			    unsigned int	      mask)
{
    if (mHoveredWindows.size () && optionGetEffectType () == HoverfxOptions::EffectTypeScale)
	mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;
    
    return gScreen->glPaintOutput (attrib, transform, region, output, mask);
    
}

bool
HoverWindow::glPaint (const GLWindowPaintAttrib &attrib,
		      const GLMatrix		&transform,
		      const CompRegion		&region,
		      unsigned int		mask)
{
    HOVER_SCREEN (screen);
  
    if (mIsHovered || mAnimate)
    {
	if (hs->optionGetEffectType () == HoverfxOptions::EffectTypeScale)
	{
	    GLMatrix wTransform (transform);
	    GLWindowPaintAttrib wAttrib (attrib);
	    
	    wTransform.translate (window->inputRect ().centerX (),
				  window->inputRect ().centerY (),
				  0.0f);
	    wTransform.scale (mCurrent, mCurrent, 1.0f);
	    wTransform.translate (-window->inputRect ().centerX (),
				  -window->inputRect ().centerY (),
				  0.0f);
	    
	    mask |= PAINT_WINDOW_TRANSFORMED_MASK;
	    
	    return gWindow->glPaint (attrib, wTransform, region, mask);
	}
	else if (hs->optionGetEffectType () == HoverfxOptions::EffectTypeOpacity)
	{
	    GLWindowPaintAttrib wAttrib (attrib);
	    wAttrib.opacity = (float) (mCurrent * OPAQUE) / 100;
	    
	    return gWindow->glPaint (wAttrib, transform, region, mask);
	}
	else if (hs->optionGetEffectType () == HoverfxOptions::EffectTypeBrightness)
	{
	    GLWindowPaintAttrib wAttrib (attrib);
	    wAttrib.brightness = (float) (mCurrent * BRIGHT) / 100;
	    
	    return gWindow->glPaint (wAttrib, transform, region, mask);
	}
	else if (hs->optionGetEffectType () == HoverfxOptions::EffectTypeSaturation)
	{
	    GLWindowPaintAttrib wAttrib (attrib);
	    wAttrib.saturation = (float) (mCurrent * COLOR) / 100;
	    
	    return gWindow->glPaint (wAttrib, transform, region, mask);
	}
    }
    
    return gWindow->glPaint (attrib, transform, region, mask);
}

void
HoverScreen::donePaint ()
{  
    foreach (CompWindow *w, mHoveredWindows)
	if (HoverWindow::get (w)->mAnimate)
	    CompositeWindow::get (w)->addDamage ();

    cScreen->donePaint ();
    
    if (!mHoveredWindows.size ())
	toggleFunctions (false);
}

bool
HoverWindow::damageRect (bool     initial,
			 const CompRect &rect)
{
    bool status = false;
    
    if (mCurrent != 1.0f)
    {
	float xTranslate, yTranslate;

	xTranslate = (window->inputRect ().width () - window->inputRect ().width () * mCurrent);
	yTranslate = (window->inputRect ().height () - window->inputRect ().height () * mCurrent);
	
	cWindow->damageTransformedRect (mCurrent * 1.1, mCurrent * 1.1,
					xTranslate / 2, yTranslate / 2, rect);
    }
    
    status |= cWindow->damageRect (initial, rect);
    
    return status;
}

HoverWindow::HoverWindow (CompWindow *w) :
    PluginClassHandler <HoverWindow, CompWindow> (w),
    window (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w)),
    mIsHovered (false),
    mAnimate (false),
    mOld (1.0f),
    mCurrent (1.0f),
    mTarget (1.0f)
{
    CompositeWindowInterface::setHandler (cWindow, false);
    GLWindowInterface::setHandler (gWindow, false);
}

HoverScreen::HoverScreen (CompScreen *s) :
    PluginClassHandler <HoverScreen, CompScreen> (s),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen))
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);
}

bool
HoverPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;
    
    return true;
}