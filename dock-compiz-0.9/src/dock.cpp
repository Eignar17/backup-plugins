#include "dock.h"

COMPIZ_PLUGIN_20090315 (dock, DockPluginVTable);


void 
DockScreen::toggleHandlers(bool state) {
  screen->handleEventSetEnabled(this, state);
}

void 
DockWindow::toggleHandlers(bool state) {
  window->moveNotifySetEnabled(this, state);
  gWindow->glPaintSetEnabled(this, state);
}


DockWindow::DockWindow (CompWindow *window) :
  PluginClassHandler <DockWindow, CompWindow> (window),
  cWindow (CompositeWindow::get (window)),
  gWindow (GLWindow::get (window)),
  window (window),
  ipw (NULL),
  grouped_l (false),
  grouped_r (false)
{
  WindowInterface::setHandler (window, false);
  CompositeWindowInterface::setHandler (cWindow, false);
  GLWindowInterface::setHandler (gWindow, false);

  /* dock all windows by default */
  DockScreen::get(screen)->setWindowDocked(this, true);
}

bool 
DockWindow::isDockable() const {
  DOCK_SCREEN(screen);

  if (window->type() &
      (CompWindowTypeDockMask |
       CompWindowTypeDesktopMask))
    return false;
  if (window->state() &
      (CompWindowStateSkipPagerMask | CompWindowStateShadedMask))
    return false;
  if (window->overrideRedirect() ||
      window->invisible() ||
      window->shaded() ||
      window->minimized())
    return false;

  if (ss->optionGetWindowMatch().evaluate(window))
    return true;

  return false;
}

DockWindow::~DockWindow () {
  DockScreen::get(screen)->setWindowDocked(this, false);
  if (ipw) delete ipw;
}


DockScreen::DockScreen (CompScreen *screen) :
  PluginClassHandler <DockScreen, CompScreen> (screen),
  cScreen (CompositeScreen::get (screen)),
  gScreen (GLScreen::get (screen)),
  grabbedWindow (None),
  grabIndex (0),
  moveCursor (XCreateFontCursor(screen->dpy(), XC_fleur)),
  docking_size (DOCKING_SIZE),
  dock_spacing (16)
{
  ScreenInterface::setHandler (screen, false);
  CompositeScreenInterface::setHandler (cScreen, false);
  GLScreenInterface::setHandler (gScreen, false);

  /* control bindings */
  optionSetToggleKeyInitiate(boost::bind(&DockScreen::toggleDock, this, _1, _2, _3));
  optionSetGroupKeyInitiate(boost::bind(&DockScreen::groupDock, this, _1, _2, _3));
  /* load backgrounds */ 
  CompString pname ("dock");
    
  CompString fname_left(/*"/home/d/src/newdock/podlozhka/v1/left.png");*/optionGetTileLeft ());
  CompString fname_center(/*"/home/d/src/newdock/podlozhka/v1/center.png");*/optionGetTileCenter ());
  CompString fname_right(/*"/home/d/src/newdock/podlozhka/v1/right.png");*/optionGetTileRight ());
  CompSize	imgSizeLeft, imgSizeCenter, imgSizeRight;
  
  //compLogMessage ("dock", CompLogLevelWarn, fname_center.c_str());
  compLogMessage ("dock", CompLogLevelWarn, optionGetTileCenter().c_str());
  
  leftTex = GLTexture::readImageToTexture (fname_left, pname, imgSizeLeft);
  centerTex = GLTexture::readImageToTexture (fname_center, pname, imgSizeCenter);
  rightTex = GLTexture::readImageToTexture (fname_right, pname, imgSizeRight);
  
//  if (!grabIndex) grabIndex = screen->pushGrab (None, "dock");
  
}

DockScreen::~DockScreen () {
  if (moveCursor)
    XFreeCursor(screen->dpy(), moveCursor);
    
  if (grabIndex) {
	screen->removeGrab (grabIndex, NULL);
	grabIndex = 0;  
  }
}

bool 
DockWindow::dock(int docked_size) {

  if (docked || !isDockable())
    return false;

  docked = true;
  toggleHandlers(true);

  //int max_dim = window->height() > window->width() ? 
  //              window->height() : window->width();
  // TODO: check for max_dim != 0
  int max_dim = window->height();
  //float additional_space_factor = 1.9;
  //wScale = docked_size / (float)max_dim / additional_space_factor;
  
  wScale = (docked_size - docked_size*0.3) / (float) max_dim;

  move_dx = move_dy = 0;

  /* create the IPW */
  if (ipw) delete ipw;
  ipw = new IPWindow(window);
 // ipw->syncIPW();
  /* damage the window */
  cWindow->addDamage();
  char ccc[90];
      sprintf(ccc,"DockWindow::dock - window %ld, ipw %ld",window,ipw);
      compLogMessage ("dock", CompLogLevelWarn, ccc);
  return true;
}


bool 
DockWindow::undock() {
  if (!docked || !isDockable())
    return false;

  docked = false;
  toggleHandlers(true);
  wScale = 1.0;

  if (ipw) {
    delete ipw;
    ipw = NULL;
  }

  /* restore undocked window position */
  window->move(-move_dx, -move_dy, true);
  window->syncPosition();

  /* damage the window */
  cWindow->addDamage();

  return true;
}

void 
DockScreen::setWindowDocked(DockWindow *w, 
			    bool       docked) {
  bool changed = false;

  if (docked) {
    if ((changed = w->dock(docking_size)))
      dockedWindows.push_back(w);
  } else {
    if ((changed = w->undock()))
      dockedWindows.remove(w);
  }

  if (changed) {
    adjustDocked();
    toggleHandlers(!dockedWindows.empty());
  }
}

bool 
DockWindow::isDocked() const {
  return docked;
}

bool 
DockScreen::groupDock(CompAction *action, CompAction::State state,
                            CompOption::Vector options) {
  CompWindow *w = screen->findWindow(screen->activeWindow());
  if (!w)
    return true;

  //DOCK_WINDOW(w);
  //setWindowDocked(sw, !sw->isDocked());
  //sw->grouped=sw->grouped?false:true;
  
  std::list<DockWindow*>::iterator it;
  CompWindow *prev_w = NULL, *cur_w = NULL; 
  DockWindow * sw;
  for (it = dockedWindows.begin(); it != dockedWindows.end(); it++) {
    prev_w = cur_w;
    cur_w = (*it)->window;
    if (cur_w == w) {// toggling grouping on the left
		if (prev_w)
		{	sw = DockWindow::get (prev_w);
			sw->grouped_r=sw->grouped_r?false:true;
			sw->cWindow->addDamage();
		}
		sw = DockWindow::get (cur_w);
		sw->grouped_l=sw->grouped_l?false:true;
		sw->cWindow->addDamage();
	}
  }
  sw->cWindow->addDamage();

  return true;
}

bool 
DockScreen::toggleDock(CompAction *action, CompAction::State state,
                            CompOption::Vector options) {
  CompWindow *w = screen->findWindow(screen->activeWindow());
  if (!w)
    return true;

  DOCK_WINDOW(w);
  setWindowDocked(sw, !sw->isDocked());



  return true;
}

void 
DockWindow::dockMove(int dx, int dy) {

  move_dx += dx; move_dy += dy;

  window->move(dx, dy, true);
  window->syncPosition();

  if (ipw) ipw->syncIPW();
}


void 
DockScreen::adjustDocked() {

  std::list<DockWindow*>::iterator it;
  int x = dock_spacing;

  for (it = dockedWindows.begin(); it != dockedWindows.end(); it++) {
    CompWindow *w = (*it)->window;

    int dx, dy;

    dx = x - w->inputRect().x();
    dy = dock_spacing - w->inputRect().y();
    (*it)->dockMove(dx, dy);

    x += (*it)->wScale * w->width() + dock_spacing;
  }
}

void 
DockScreen::refreshDock(DockWindow *w) {
  std::list<DockWindow*>::iterator it;
  bool inserted = false;
  
  for (it = dockedWindows.begin(); it != dockedWindows.end(); it++) {
    CompWindow *win = (*it)->window;
    
    if (w->window->id () == win->id()) {
      dockedWindows.erase(it);			
      break;
    }
  }
  
  int x = dock_spacing;
  for (it = dockedWindows.begin(); it != dockedWindows.end(); it++) {
    CompWindow *win = (*it)->window;
    
    // TODO:
    // Need to check mouse coordinate
    if (w->window->serverX() <= x) {
      dockedWindows.insert(it, w);	
      x += (*it)->wScale * win->width() + dock_spacing;
      inserted = true;
      break;
    }
    x += (*it)->wScale * win->width() + dock_spacing;
  }	
  if (!inserted) {
    dockedWindows.insert(it, w);    
  }
}
/*
CompWindow *
DockScreen::findRealWindowID (Window wid) {
    CompWindow *orig;

    orig = screen->findWindow (wid);
    if (!orig)
	return NULL;

    return DockWindow::get (orig)->getRealWindow ();
}*/

/* Checks if w is a ipw and returns the real window */
/*CompWindow *
DockWindow::getRealWindow ()
{
    WindowInfo *run;

    SHELF_SCREEN (screen);

    foreach (run, ss->shelfedWindows)
    {
	if (window->id () == run->ipw)
	    return run->w;
    }

    return NULL;
}*/

void 
DockScreen::handleEvent(XEvent *ev) {
  CompWindow *w;
//compLogMessage ("dock", CompLogLevelWarn, "event");
  switch (ev->type) {

    case EnterNotify:
      if ((w = IPW2OverridenWin(ev->xcrossing.window))) {
		  compLogMessage ("dock", CompLogLevelWarn, "enter");
        XEvent e;
        memcpy(&e.xcrossing, &ev->xcrossing, sizeof(XCrossingEvent));
        e.xcrossing.window = w->frame() ? w->frame() : w->id();
        XSendEvent(screen->dpy(), w->id(), false, EnterWindowMask, &e);
        if (!grabIndex) grabIndex = screen->pushGrab (None, "dock");
      }
      break;
    case LeaveNotify:
      if ((w = IPW2OverridenWin(ev->xcrossing.window))) {
		  compLogMessage ("dock", CompLogLevelWarn, "leave");
        XEvent e;
        memcpy(&e.xcrossing, &ev->xcrossing, sizeof(XCrossingEvent));
        e.xcrossing.window = w->frame() ? w->frame() : w->id();
        XSendEvent(screen->dpy(), w->id(), false, LeaveWindowMask, &e);
        if (grabIndex) {
	      screen->removeGrab (grabIndex, NULL);
	      grabIndex = 0;  
        }
      }
      break;
    case MotionNotify:
      compLogMessage ("dock", CompLogLevelWarn, "motion");
      if (grabIndex && (w = screen->findWindow(grabbedWindow))) {
        int dx = ev->xmotion.x_root - motion_lastx;
        int dy = ev->xmotion.y_root - motion_lasty;

        DockWindow::get(w)->dockMove(dx, dy);
        motion_lastx += dx;
        motion_lasty += dy;
      }
      break;

    case ButtonPress:
	  compLogMessage ("dock", CompLogLevelWarn, "press");
	  
	 
  
      if ((w = IPW2OverridenWin(ev->xbutton.window))) {
		  compLogMessage ("dock", CompLogLevelWarn, "press ipw");
        if (!screen->otherGrabExist("dock", 0)) {
          w->activate();
          grabbedWindow = w->id();
          grabIndex = screen->pushGrab(moveCursor, "dock");
          motion_lastx = ev->xbutton.x_root;
          motion_lasty = ev->xbutton.y_root;
        }
      }
      break;

    case ButtonRelease:
    compLogMessage ("dock", CompLogLevelWarn, "release");
    //CompWindow *highlightedWindow = getHoveredWindow ();
    //w = screen->findWindow (wid);
    //CompWindow *w;
    w = screen->findWindow (ev->xbutton.window);
    char ccc[50];
    sprintf(ccc,"ButtonRelease: release window is %ld",w);
     compLogMessage ("dock", CompLogLevelWarn, ccc);
    if ((w = IPW2OverridenWin(ev->xcrossing.window))) {
	//char ccc[50];
    sprintf(ccc,"ButtonRelease: window x is %d",w->inputRect().x());
    compLogMessage ("dock", CompLogLevelWarn, ccc);
    sprintf(ccc,"ButtonRelease: current x is %d",ev->xbutton.x_root);
    compLogMessage ("dock", CompLogLevelWarn, ccc); }	
      if ((w = screen->findWindow(grabbedWindow))) {
	// TODO:
	// Need to get mose x,y coordinate and put them to refreshDock();

    refreshDock(DockWindow::get(w));
        setWindowDocked(DockWindow::get(w), true);
				adjustDocked();
				grabbedWindow = None;
        if (grabIndex) {
          w->moveInputFocusTo();
          screen->removeGrab(grabIndex, NULL);
          grabIndex = 0;
        }
      }
			
      break;

  }

  screen->handleEvent(ev);

  switch (ev->type) {
    case ConfigureNotify:

      /* xconfigure.above holds the window which xconfigure.window is above of */

      std::list<DockWindow*>::iterator it;
      for (it = dockedWindows.begin();
          it != dockedWindows.end(); it++) {

        if ((*it)->ipw->overridenWin == ev->xconfigure.window) {
          if (ev->xconfigure.above != (*it)->prev) {
            (*it)->prev = ev->xconfigure.above;
            (*it)->ipw->syncIPW();
          }
        } 

        if ((*it)->ipw->overridenWin == ev->xconfigure.above) {
          if (ev->xconfigure.window != (*it)->next) {
            (*it)->next = ev->xconfigure.window;
            (*it)->ipw->syncIPW();
          }
        }
      }
      break;
  }

}


bool 
DockWindow::glPaint(const GLWindowPaintAttrib &attrib,
                         const GLMatrix &transform, 
                         const CompRegion &region, unsigned int mask) {

  bool status;
  DOCK_SCREEN (screen);
  GLWindowPaintAttrib sAttrib = attrib;
  
  if (docked) {

    float tx, ty;
    GLTexture* tex;
    int tex_width;
    tx = window->input().left * (wScale - 1.0);
   //tx = window->x()*wScale;
    ty = window->input().top * (wScale - 1.0);


// paint tile
	// left part:
	if (grouped_l) {
		tex=ss->centerTex[0]; tex_width=ceilf(ss->dock_spacing/2.0); //for grouped
	} else {
		tex=ss->leftTex[0]; tex_width=tex->width(); //for non-grouped
	}
	
	if(1){	GLTexture::MatrixList matl;
		CompRegion texReg(0, 0, tex_width, ss->docking_size*1.2);
		gWindow->geometry ().reset ();
    	matl.push_back (tex->matrix ());
		gWindow->glAddGeometry (matl, texReg, texReg);
	
		if (gWindow->geometry ().vCount)
		{
		    GLFragment::Attrib	fragment (sAttrib);
		    GLMatrix		wTransform (transform);

		    fragment = GLFragment::Attrib (sAttrib);
			wTransform.translate(window->x(), window->y(), 0.0);

			wTransform.translate (tx, -ss->dock_spacing*1.7, 0.0);

			mask |= PAINT_WINDOW_BLEND_MASK;

		    glPushMatrix ();
		    glLoadMatrixf (wTransform.getMatrix ());

		    gWindow->glDrawTexture (tex, fragment, mask);

		    glPopMatrix ();
		
		}
    }
	//central part
	tex=ss->centerTex[0]; 
	{	GLTexture::MatrixList matl;
		CompRegion texReg(0, 0, window->width()*wScale, ss->docking_size*1.2);
		gWindow->geometry ().reset ();
    	matl.push_back (tex->matrix ());
		gWindow->glAddGeometry (matl, texReg, texReg);
	
		if (gWindow->geometry ().vCount)
		{
		    GLFragment::Attrib	fragment (sAttrib);
		    GLMatrix		wTransform (transform);

		    fragment = GLFragment::Attrib (sAttrib);
			wTransform.translate(window->x(), window->y(), 0.0);

			wTransform.translate (tx+tex_width,-ss->dock_spacing*1.7, 0.0);
			mask |= PAINT_WINDOW_BLEND_MASK;

		    glPushMatrix ();
		    glLoadMatrixf (wTransform.getMatrix ());

		    gWindow->glDrawTexture (tex, fragment, mask);

		    glPopMatrix ();
		
		}
    }
    //right part
	
	if (grouped_r) {
		tex=ss->centerTex[0]; tex_width=floorf(ss->dock_spacing/2.0);// for grouped 
	} else {
		tex=ss->rightTex[0]; tex_width=tex->width();// for non-grouped
	}
	if(1) {	GLTexture::MatrixList matl;
		CompRegion texReg(0, 0, tex_width, ss->docking_size*1.2);
		gWindow->geometry ().reset ();
    	matl.push_back (tex->matrix ());
		gWindow->glAddGeometry (matl, texReg, texReg);
	
		if (gWindow->geometry ().vCount)
		{
		    GLFragment::Attrib	fragment (sAttrib);
		    GLMatrix		wTransform (transform);

		    fragment = GLFragment::Attrib (sAttrib);
			wTransform.translate(window->x(), window->y(), 0.0);

			wTransform.translate (floorf(tx+tex_width+window->width()*wScale), 
			-ss->dock_spacing*1.7, 0.0);
			
			mask |= PAINT_WINDOW_BLEND_MASK;
		    glPushMatrix ();
		    glLoadMatrixf (wTransform.getMatrix ());

		    gWindow->glDrawTexture (tex, fragment, mask);
		    glPopMatrix ();
		}
    }
// paint mini-window
    GLMatrix mTrans = transform;
 
    mTrans.translate(window->x()/*+ss->dock_spacing/1.5*/, window->y()+ss->docking_size*0.15, 0.0);

    mTrans.scale(wScale, wScale, 1.0);
    mTrans.translate((tx+tex_width)/wScale/*tx / wScale*/- window->x(), 
                     ty / wScale - window->y(), 0.0);

    
	mask |= PAINT_WINDOW_TRANSFORMED_MASK;

	status = gWindow->glPaint(attrib, mTrans, region, mask);

// paint icon
	GLTexture *icon;
    icon = gWindow->getIcon (96, 96);
	if (!icon) icon = ss->gScreen->defaultIcon ();

    if (icon && ss->optionGetIcons()) // have some icon and checkbox in options
    {	GLTexture::MatrixList matl;
		CompRegion iconReg(0, 0, icon->width (), icon->height ());
		gWindow->geometry ().reset ();
    	matl.push_back (icon->matrix ());
		gWindow->glAddGeometry (matl, iconReg, iconReg);
	
		if (gWindow->geometry ().vCount)
		{
		    GLFragment::Attrib	fragment (sAttrib);
		    GLMatrix		wTransform (transform);

		    fragment = GLFragment::Attrib (sAttrib);

		    wTransform.translate (tx, ty, 0.0);

			wTransform.translate (window->x () +
				     window->width () * wScale - icon->width () * 0.5 +ss->dock_spacing/1.5,
				     window->y ()+ss->docking_size*0.15 + 
				     window->height () *  wScale -icon->height () * 0.5 / 2.0, 0.0f);

		   wTransform.scale(0.5, 0.5, 1.0);

			mask |= PAINT_WINDOW_BLEND_MASK;

		    glPushMatrix ();
		    glLoadMatrixf (wTransform.getMatrix ());

		    gWindow->glDrawTexture (icon, fragment, mask);

		    glPopMatrix ();
		
		}
    }

    
  } else {
    status = gWindow->glPaint(attrib, transform, region, mask);
  }
  return status;
}


bool 
DockScreen::glPaintOutput(const GLScreenPaintAttrib &attrib, 
    const GLMatrix &transform, const CompRegion &region,
    CompOutput *output, unsigned int mask) {

  if (!dockedWindows.empty())
    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

  return gScreen->glPaintOutput (attrib, transform, region, output, mask);
}


bool 
DockPluginVTable::init () {

  if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
      !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
      !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
    return false;

  // TODO: check for XShape xtension

  return true;
}
