#include <compiz-core.h>
#include <compiz-plugin.h>
#include <compiz-animation.h>

#include <math.h>

#include "drunken_options.h"

extern int displayPrivateIndex;

typedef struct _DrunkenDisplay
{
    int screenPrivateIndex;
} DrunkenDisplay;

typedef struct _DrunkenScreen
{
    int windowPrivateIndex;

    bool mEnabled;

    void toggleFunctions (bool);

    Bool toggle ();

    PreparePaintScreenProc preparePaintScreen;
    PaintOutputProc        paintOutput;
    DonePaintScreenProc    donePaintScreen;
} DrunkenScreen;

typedef struct _DrunkenWindow
{
        Bool shouldAnimate ();

        float mDrunkFactor;
};

#define GET_DRUNK_DISPLAY(d)                            \
    ((DrunkenDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define GET_DRUNK_SCREEN(s, dd)                         \
    ((DrunkenScreen *) (s)->base.privates[(dd)->screenPrivateIndex].ptr)

#define GET_DRUNK_WINDOW(w, ds)                           \
    ((DrunkenWindow *) (w)->base.privates[(ds)->windowPrivateIndex].ptr)

#define DRUNK_DISPLAY(d)						       \
    DrunkenDisplay *dd = GET_DRUNK_DISPLAY (d)

#define DRUNK_SCREEN(s)						       \
   DrunkenScreen *ds = GET_DRUNK_SCREEN (s, GET_DRUNK_DISPLAY (s->display))

#define DRUNK_WINDOW(w)							\
    DrunkenWindow *dw = GET_DRUNK_WINDOW (w, GET_DRUNK_SCREEN (w->screen, GET_DRUNK_DISPLAY (w->screen->display)))

#endif
