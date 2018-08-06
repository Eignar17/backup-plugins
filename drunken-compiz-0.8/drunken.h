#include <compiz-core.h>
#include <compiz-plugin.h>

#include <math.h>

#include "drunken_options.h"

extern int displayPrivateIndex;

typedef struct _DrunkenDisplay
{
    int screenPrivateIndex;

typedef struct _DrunkenScreen
{
    int windowPrivateIndex;

    PaintOutputProc              paintOutput;
	
	bool		mEnabled;
	
	void		toggleFunctions (bool);

    PaintWindowProc paintWindow;

	Bool toggle ();

};

typedef struct _DrunkenWindow DrunkenWindow;
	  
	  
	  Bool shouldAnimate ();

	  Bool
	  glPaint (const ScreenPaintAttrib &,
		   const CompMatrix	     &,
		   const Region	             &,
		   unsigned int		      );

	  float	drunkenGetFactor;
};

#define GET_STEREO3D_DISPLAY(d)                            \
    ((DrunkeDisplay *) (d)->base.privates[displayPrivateIndex].ptr)


#define GET_DRUNK_SCREEN(s, ds)                         \
    ((DrunkenScreen *) (s)->base.privates[(ds)->screenPrivateIndex].ptr)

#define GET_DRUNK_WINDOW(w, dw)                         \
    ((DrunkenWindow *) (w)->base.privates[(dw)->windowPrivateIndex].ptr)

#define DRUNK_DISPLAY(d)						       \
    Stereo3DDisplay *sod = GET_DRUNK_DISPLAY (d)

#define DRUNK_SCREEN(s)						       \
    Stereo3DScreen *ds = GET_DRUNK_SCREEN (s, GET_DRUNK_DISPLAY (s->display))

#define DRUNK_WINDOW(w)							\
    DrunkenWindow *dw = GET_DRUNK_WINDOW (w, GET_DRUNK_SCREEN (w->screen, GET_SDRUNK_DISPLAY (w->screen->display)))

#endif
