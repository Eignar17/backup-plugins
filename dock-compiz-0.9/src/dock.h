/*
 * Compiz dock-panel plugin
 *
 * dock.h
 *
 * Based on code dock plugin: 
 * Rodolfo Granata <warlock.cc@gmail.com>
 * 
 * Autors:
 * Vladimir Diomin <spas.work@gmail.com>
 * Dmitriy Kostiuk <dmitriykostiuk@gmail.com>
 *
 * Description:
 * Mini-windows panel for windows manipulating
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _DOCK_H_
#define _DOCK_H_

#include <X11/extensions/shape.h>
#include <X11/cursorfont.h>

#include <cmath>

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include "dock_options.h"
#include "ipw.h"

#define DOCKING_SIZE 151.0 // pixel size of docked windows

#define DOCK_SCREEN(s)							       \
	DockScreen *ss = DockScreen::get (s)

#define DOCK_WINDOW(w)							       \
	DockWindow *sw = DockWindow::get (w)


class DockWindow :
  public PluginClassHandler <DockWindow, CompWindow>,
  public WindowInterface,
  public CompositeWindowInterface,
  public GLWindowInterface
{
  public:
    DockWindow (CompWindow *);
    ~DockWindow ();

    bool isDocked() const;
    bool isDockable() const;
    bool dock(int docked_size);
    bool undock(); 

    void dockMove(int dx, int dy);

  //private:

    CompositeWindow *cWindow;
    GLWindow	*gWindow;
    CompWindow *window;
    IPWindow *ipw;

    float wScale;
    bool docked;
	bool grouped_l, grouped_r;
    /* undo vector */
    int move_dx, move_dy;

    /* restacking checks */
    Window prev, next;

    void toggleHandlers(bool state);

    /* wraped handlers */
    bool glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
                  const CompRegion &, unsigned int);
    //CompWindow * getRealWindow ();
};

class DockScreen :
  public PluginClassHandler <DockScreen, CompScreen>,
  public ScreenInterface,
  public CompositeScreenInterface,
  public GLScreenInterface,
  public DockOptions
{
  public:

    DockScreen (CompScreen *);
    ~DockScreen ();

    void setWindowDocked(DockWindow *w, bool docked);

    void refreshDock(DockWindow *w);

  //private:

    CompositeScreen *cScreen;
    GLScreen	*gScreen;

    Window grabbedWindow;
    CompScreen::GrabHandle grabIndex;
    Cursor moveCursor;

    int motion_lastx, motion_lasty;

    /* list of currently docked windows */
    std::list<DockWindow*> dockedWindows;
    /* Normalized size for docked windows */
    int docking_size;
    /* docked windows spacing */
    int dock_spacing;
	/* textures for dock substrate image */
	GLTexture::List leftTex, centerTex, rightTex;
	//CompSize	imgSizeLeft, imgSizeCenter, imgSizeRigth;
    
    void adjustDocked();

    bool toggleDock(CompAction *action, CompAction::State state,
                    CompOption::Vector options);
                    
     bool groupDock(CompAction *action, CompAction::State state,
                    CompOption::Vector options);

    void toggleHandlers(bool state);

    /* wraped handlers */
    void handleEvent (XEvent *);

    bool glPaintOutput (const GLScreenPaintAttrib &, const GLMatrix &,
        const CompRegion &, CompOutput *, unsigned int);
    
    //CompWindow * findRealWindowID (Window wid);

  private:

};


class DockPluginVTable :
  public CompPlugin::VTableForScreenAndWindow <DockScreen, DockWindow>
{
  public:
    bool init ();
};

#endif /* _DOCK_H_ */
