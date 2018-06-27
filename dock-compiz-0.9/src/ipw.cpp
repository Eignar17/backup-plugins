#include "ipw.h"
#include "dock.h"

void IPWindow::clearOverridenShape() {
  /* first retrieve current shape and save that fore later */
  winRects = XShapeGetRectangles(screen->dpy(), overridenWin,
                                 ShapeInput, &nRects, &rOrdering);

  /* if returned shape matches exactly window shape =>
   * window hasn't being shaped */
  if (nRects == 1 &&
      winRects[0].x == -compWin->serverGeometry().border() &&
      winRects[0].y == -compWin->serverGeometry().border() &&
      winRects[0].width == compWin->serverWidth() + 
                            2 * compWin->serverGeometry().border() &&
      winRects[0].height == compWin->serverHeight() + 
                            2 * compWin->serverGeometry().border()) {
    nRects = 0;
  }

  /* clear shape */
  XShapeSelectInput(screen->dpy(), overridenWin, NoEventMask);
  XShapeCombineRectangles(screen->dpy(), overridenWin, ShapeInput,
                          0, 0, NULL, 0, ShapeSet, 0);
  XShapeSelectInput(screen->dpy(), overridenWin, ShapeNotify);
}

void IPWindow::restoreOverridenShape() {
  if (nRects) {
    XShapeCombineRectangles(screen->dpy(), overridenWin, 
                            ShapeInput, 0, 0, winRects,
                            nRects, ShapeSet, rOrdering);
  } else {
    XShapeCombineMask(screen->dpy(), overridenWin,
                      ShapeInput, 0, 0, None, ShapeSet);
  }

  if (winRects) {
    XFree(winRects);
    nRects = -1;
    rOrdering = 0;
  }
}


/* put the IPW where the real window is */
void IPWindow::syncIPW() {
  XWindowChanges xwc;

  DOCK_WINDOW(compWin);

  xwc.x = compWin->inputRect().x();
  xwc.y = compWin->inputRect().y();
  xwc.width = compWin->inputRect().width() * sw->wScale;
  xwc.height = compWin->inputRect().height() * sw->wScale;
  xwc.stack_mode = Below;
  xwc.sibling = overridenWin;

  char ccc[50];
  sprintf(ccc,"syncIPW: overridenWin is %ld",overridenWin);
  compLogMessage ("dock", CompLogLevelWarn, ccc);

  XConfigureWindow(screen->dpy(), ipwWin,
    CWSibling | CWStackMode | CWX | CWY | CWWidth | CWHeight, &xwc);
 //   XMapWindow (screen->dpy(), ipwWin);
}


IPWindow::IPWindow(CompWindow *cw) : compWin(cw) {
  XSetWindowAttributes attrib;

  /* select windo to override */
  overridenWin = cw->frame() ? cw->frame() : cw->id();

  clearOverridenShape();

  /* Create input prevention window (IPW) */
  attrib.override_redirect = true;
  attrib.event_mask = 0;

  ipwWin = XCreateWindow(screen->dpy(), screen->root(), 0, 0, 100, 100,
                         0, CopyFromParent, InputOnly, CopyFromParent,
                         CWEventMask | CWOverrideRedirect, &attrib);

// ipwWin = XCreateWindow (screen->dpy(), screen->root(),
//			 cw->serverX - cw->input.left,
//			 cw->serverY - cw->input.top,
//			 cw->serverWidth + cw->input.left + cw->input.right,
//			 cw->serverHeight + cw->input.top + cw->input.bottom,
//			 0, CopyFromParent, InputOnly, CopyFromParent,
//			 CWEventMask | CWOverrideRedirect,
//			 &attrib);

  XMapWindow(screen->dpy(), ipwWin);
  /* put the IPW over the real window */
  syncIPW();
}

IPWindow::~IPWindow() {
  restoreOverridenShape();
  XUnmapWindow(screen->dpy(), ipwWin);
  XDestroyWindow(screen->dpy(), ipwWin);
}



CompWindow *IPW2OverridenWin(Window win) {

  DOCK_SCREEN(screen);

  std::list<DockWindow *>::iterator it;

      char ccc[50];
      sprintf(ccc,"IPW2Over: win is %ld",win);
      compLogMessage ("dock", CompLogLevelWarn, ccc);

  for (it = ss->dockedWindows.begin(); 
       it != ss->dockedWindows.end(); it++)
  {		sprintf(ccc,"IPW2Over: dockedWin is %ld",(*it)->window);
        compLogMessage ("dock", CompLogLevelWarn, ccc);
	    if ( ((*it)->ipw) && ((*it)->ipw->overridenWin/*ipwWin*/ == win) )
        return (*it)->window;
  }
  /* not an IPW window */
  return NULL;
}
//DockWindow::dock - window 21968176, ipw 26791920
