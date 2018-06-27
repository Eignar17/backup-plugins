/* 
 * Compiz dock-panel plugin
 */

#ifndef _IPW_H_
#define _IPW_H_

#include <core/core.h>

class IPWindow {
  public:

    IPWindow(CompWindow *);
    ~IPWindow();

    void syncIPW();

  //private:

    void clearOverridenShape();
    void restoreOverridenShape();

    /* actual window we're preventing input to */
    CompWindow *compWin;
    /* Xwindow we're preventing input to */
    Window overridenWin;
    /* input prevention window */
    Window ipwWin;

    /* saved shape */
    XRectangle *winRects;
    int nRects;
    int rOrdering;
};

CompWindow *IPW2OverridenWin(Window ipw);

#endif /* _IPW_H_ */
