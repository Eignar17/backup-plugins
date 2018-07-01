#include <compiz-core.h>
#include "basicblur_options.h"

#include <math.h>
#include <GL/glext.h>

#ifndef PAINT_WINDOW_DECORATION_MASK
#define PAINT_WINDOW_DECORATION_MASK (1 << 4)
#endif

static int displayPrivateIndex;

static int corePrivateIndex;

typedef struct _basicblurCore {
    ObjectAddProc objectAdd;
} basicblurCore;

static int BasicBlurdisplayPrivateIndex;

static GLfloat * convfilter;
static int fsize = 3 ;// should be an option. Hackers please help :D


typedef struct _basicblurDisplay {
    int		    screenPrivateIndex;
} basicblurDisplay;


typedef struct _basicblurScreen {
    int                          windowPrivateIndex;
    PaintWindowProc              paintWindow;
    DrawWindowProc	         drawWindow;
    DonePaintScreenProc	         donePaintScreen;
    PaintTransformedOutputProc	 paintTransformedOutput;
    PaintOutputProc	         paintOutput;
    DrawWindowTextureProc        drawWindowTexture;

    WindowResizeNotifyProc       windowResizeNotify;
    WindowMoveNotifyProc         windowMoveNotify;

    Bool                         isBlured;
} basicblurScreen;

typedef struct _basicblurWindow {
    Bool first;
    Bool isBlured;
    Region region;
} basicblurWindow;

#define GET_BASICBLUR_CORE(c)				    \
    ((basicblurCore *) (c)->base.privates[corePrivateIndex].ptr)

#define BASICBLUR_CORE(c)		     \
    basicblurCore *nc = GET_BASICBLUR_CORE (c)

#define GET_BASICBLUR_DISPLAY(d)					  \ 
    ((basicblurDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define BASICBLUR_DISPLAY(d)			   \
    basicblurDisplay *nd = GET_BASICBLUR_DISPLAY (d)

#define GET_BASICBLUR_SCREEN(s, nd)					      \
    ((basicblurScreen *) (s)->base.privates[(nd)->screenPrivateIndex].ptr)

#define BASICBLUR_SCREEN(s)							\
    basicblurScreen *ns = GET_BASICBLUR_SCREEN (s, GET_BASICBLUR_DISPLAY (s->display))

#define GET_BASICBLUR_WINDOW(w, ns)					      \
    ((basicblurWindow *) (w)->base.privates[(ns)->windowPrivateIndex].ptr)

#define BASICBLUR_WINDOW(w)		              \
    basicblurWindow *nw = GET_BASICBLUR_WINDOW  (w,		     \
		     GET_BASICBLUR_SCREEN  (w->screen,	     \
		     GET_BASICBLUR_DISPLAY (w->screen->display)))

#define WIN_OUTPUT_X(w) (w->attrib.x - w->output.left)
#define WIN_OUTPUT_Y(w) (w->attrib.y - w->output.top)

#define WIN_OUTPUT_W(w) (w->width + w->output.left + w->output.right)
#define WIN_OUTPUT_H(w) (w->height + w->output.top + w->output.bottom)

#define NUM_OPTIONS(s) (sizeof ((s)->opt) / sizeof (CompOption))

#define DAMAGE_WIN_REGION \
    REGION region; \
    region.rects = &region.extents; \
    region.numRects = region.size = 1; \
    region.extents.x1 = WIN_OUTPUT_X (w); \
    region.extents.x2 = WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w); \
    region.extents.y1 = WIN_OUTPUT_Y (w); \
    region.extents.y2 = WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w); \
    damageScreenRegion (w->screen, &region);
    

static void
basicblurToggleWindow (CompWindow *w)
{
    if((w->type & CompWindowTypeNormalMask) ||
       (w->type & CompWindowTypeDesktopMask))
    {
       BASICBLUR_WINDOW (w);
       nw->isBlured = !nw->isBlured;
       nw->first = TRUE;
      /*
      if(w->redirected)
      {
      	unredirectWindow(w);
      	redirectWindow(w);
      }
      */
      addWindowDamage(w);

    DAMAGE_WIN_REGION
   }
}

static void
basicblurToggleScreen (CompScreen *s)
{
    CompWindow *w;
    BASICBLUR_SCREEN (s);

    ns->isBlured = !ns->isBlured;
    for (w = s->windows; w; w = w->next)
        if (w)
        {
            BASICBLUR_WINDOW (w);
            addWindowDamage(w);
            basicblurToggleWindow (w);


            nw->region->rects = &nw->region->extents;
            nw->region->numRects = 1;

            nw->region->extents.x1 = WIN_OUTPUT_X (w);
            nw->region->extents.x2 = WIN_OUTPUT_X (w) + WIN_OUTPUT_W (w);
            nw->region->extents.y1 = WIN_OUTPUT_Y (w);
            nw->region->extents.y2 = WIN_OUTPUT_Y (w) + WIN_OUTPUT_H (w);

            damageScreenRegion (w->screen, nw->region);


            DAMAGE_WIN_REGION;
        }
}

static Bool
basicblurToggle (CompDisplay *d, CompAction *action, CompActionState state, CompOption *option, int nOption)
{
    CompWindow *w;
    Window xid;

    fprintf(stderr, "\nbasicblurToggle");

    xid = getIntOptionNamed (option, nOption, "window", 0);

    w = findWindowAtDisplay (d, xid);

    if (w)
        basicblurToggleWindow (w);

    return TRUE;
}

static Bool
basicblurToggleAll (CompDisplay *d, CompAction *action, CompActionState state, CompOption *option, int nOption)
{
    CompScreen *s;
    Window xid;

    fprintf(stderr, "\nbasicblurToggleAll");

    xid = getIntOptionNamed (option, nOption, "root", 0);

    s = findScreenAtDisplay (d, xid);

    // We invert the active window.
    // When calling the toggle all, the active window gets
    // the focus because it will be the only one without blur.
    Window xid2 = getIntOptionNamed (option, nOption, "window", 0);
    CompWindow * w = findWindowAtDisplay (d, xid2);

    if((w->type & CompWindowTypeNormalMask) ||
       (w->type & CompWindowTypeDesktopMask))
    {
       BASICBLUR_WINDOW (w);
       nw->isBlured = !nw->isBlured;
       nw->first = TRUE;
    }

    if (s)
        basicblurToggleScreen (s);

    return TRUE;
}

static Bool
basicblurPaintWindow (CompWindow		 *w,
		 const WindowPaintAttrib *attrib,
		 const CompTransform	 *transform,
		 Region			 region,
		 unsigned int		 mask)
{
    CompScreen	      *s = w->screen;
    Bool	      status;

    BASICBLUR_WINDOW (w);
    BASICBLUR_SCREEN (w->screen);

    if(nw->isBlured)
    {
        // We use a convolution filter
        glEnable(GL_CONVOLUTION_2D);
        glConvolutionFilter2D (GL_CONVOLUTION_2D, GL_RGBA, fsize, fsize, GL_RGBA, GL_FLOAT, convfilter);
        glConvolutionParameteri (GL_CONVOLUTION_2D, GL_CONVOLUTION_BORDER_MODE, GL_REPLICATE_BORDER);
        UNWRAP (ns, w->screen, paintWindow);
        status = (*s->paintWindow) (w, attrib, transform, region, mask);
        WRAP (ns, w->screen, paintWindow, basicblurPaintWindow);
        if(nw->first)
        {
           nw->first = FALSE;
        }
        glDisable(GL_CONVOLUTION_2D);

    }
    else
    {
        UNWRAP (ns, w->screen, drawWindowTexture);
        status = (*s->paintWindow) (w, attrib, transform, region, mask);
        WRAP (ns, w->screen, paintWindow, basicblurPaintWindow);
    }

    return status;
}

static void
basicblurDrawWindowTexture (CompWindow		*w,
  		      CompTexture		*texture,
		      const FragmentAttrib      *attrib,
		      unsigned int		mask)
{
    BASICBLUR_SCREEN (w->screen);
    BASICBLUR_WINDOW (w);

    //fsize = basicblurGetBlurFactor(w->screen);
    
    //fprintf(stderr, "%i", fsize);

    if(nw->isBlured)
    {
        // We use a convolution filter
        glEnable(GL_CONVOLUTION_2D);
        glConvolutionFilter2D (GL_CONVOLUTION_2D, GL_RGBA, fsize, fsize, GL_RGBA, GL_FLOAT, convfilter);
        glConvolutionParameteri (GL_CONVOLUTION_2D, GL_CONVOLUTION_BORDER_MODE, GL_REPLICATE_BORDER);
        UNWRAP (ns, w->screen, drawWindowTexture);
        (*w->screen->drawWindowTexture) (w, w->texture, attrib, mask);
        WRAP (ns, w->screen, drawWindowTexture, basicblurDrawWindowTexture);
        if(nw->first)
        {
           nw->first = FALSE;
        }
        glDisable(GL_CONVOLUTION_2D);

    }
    else
    {
        UNWRAP (ns, w->screen, drawWindowTexture);
        (*w->screen->drawWindowTexture) (w, texture, attrib, mask);
        WRAP (ns, w->screen, drawWindowTexture, basicblurDrawWindowTexture);
    }

    DAMAGE_WIN_REGION;
}

static Bool 
basicblurDamageWindowRect (CompWindow *w, Bool initial, BoxPtr rect)
{
    int status;

    BASICBLUR_SCREEN (w->screen);
    BASICBLUR_WINDOW (w);

    if (initial)
    {
        if (ns->isBlured && !nw->isBlured)
            basicblurToggleWindow (w);
    }


    DAMAGE_WIN_REGION;


    UNWRAP (ns, w->screen, damageWindowRect);
    status = (*w->screen->damageWindowRect) (w, initial, rect);
    WRAP (ns, w->screen, damageWindowRect, basicblurDamageWindowRect);

    return status;
}

// ------------------------------------------------------------------------------- OBJECT ADD

static void
basicblurObjectAdd (CompObject *parent,
			CompObject *object)
{
    static ObjectAddProc dispTab[] = {
	(ObjectAddProc) 0, /* CoreAdd */
        (ObjectAddProc) 0, /* DisplayAdd */
        (ObjectAddProc) 0, /* ScreenAdd */
        (ObjectAddProc) basicblurWindowAdd
    };

    BASICBLUR_CORE (&core);

    UNWRAP (nc, &core, objectAdd);
    (*core.objectAdd) (parent, object);
    WRAP (nc, &core, objectAdd, basicblurObjectAdd);

    DISPATCH (object, dispTab, ARRAY_SIZE (dispTab), (parent, object));
}

// ------------------------------------------------------------------------------- CORE

static Bool
basicblurInitCore (CompPlugin *p,
	     CompCore   *c)
{
    basicblurCore *nc;

    if (!checkPluginABI ("core", CORE_ABIVERSION))
        return FALSE;

    ac = malloc (sizeof (basicblurCore));
    if (!nc)
        return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
        free (nc);
        return FALSE;
    }

    WRAP (nc, c, objectAdd, basicblurObjectAdd);

    c->base.privates[corePrivateIndex].ptr = nc;

    return TRUE;
}


static void
basicblurFiniCore (CompPlugin *p,
			CompCore   *c)
{
    BASICBLUR_CORE (c);

    freeDisplayPrivateIndex (displayPrivateIndex);

    UNWRAP (nc, c, objectAdd);

    free (nc);
}

// ------------------------------------------------------------------------------- DISPLAY

static Bool
basicblurInitDisplay (CompPlugin  *p, CompDisplay *d)
{
    basicblurDisplay *nd;

    nd = malloc (sizeof (basicblurDisplay));
    if (!nd)
	return FALSE;

    nd->screenPrivateIndex = allocateScreenPrivateIndex (d);
    if (nd->screenPrivateIndex < 0)
    {
	free (nd);
	return FALSE;
    }

    d->base.privates[BasicBlurdisplayPrivateIndex].ptr = nd;
    
    basicblurSetWindowToggleKeyInitiate (d, basicblurToggle);
    basicblurSetScreenToggleKeyInitiate (d, basicblurToggleAll);

    return TRUE;
}

static void
basicblurFiniDisplay (CompPlugin  *p, CompDisplay *d)
{
    BASICBLUR_DISPLAY (d);
    freeScreenPrivateIndex (d, nd->screenPrivateIndex);
    free (nd);
    
}

static Bool
basicblurInitScreen (CompPlugin *p, CompScreen *s)
{
    basicblurScreen *ns;
    BASICBLUR_DISPLAY (s->display);

    ns = malloc (sizeof (basicblurScreen));
    if (!ns)
	return FALSE;

    ns->windowPrivateIndex = allocateWindowPrivateIndex (s);
    if (ns->windowPrivateIndex < 0)
    {
	free (ns);
	return FALSE;
    }

    ns->isBlured = FALSE;

    WRAP (ns, s, drawWindowTexture, basicblurDrawWindowTexture);
    WRAP (ns, s, damageWindowRect, basicblurDamageWindowRect);
    WRAP (ns, s, paintWindow, basicblurPaintWindow);
    
    // We fill in the convolution filter (gaussian blur)
    
    //fsize = basicblurGetBlurFactor(s);
    
    //fprintf(stderr, "%i", fsize);
    
    unsigned int i = 0;
    unsigned int j = 0;
    double k = (((double)fsize)-1.)/2.;
    double somme = 0.;
    double sigma2 = fsize * fsize / 4.;
    convfilter = malloc(4 * fsize * fsize * sizeof(GLfloat));
    GLfloat * gaussien = malloc(fsize * fsize * sizeof(GLfloat));

    for(i=0; i<fsize;i++)
       for(j=0; j<fsize;j++)
       {
          gaussien[fsize * i + j]=exp(-((double)(i-k)*(i-k)+(j-k)*(j-k))/
                                    (2.*sigma2));
       }
    for(i=0; i<fsize;i++)
       for(j=0; j<fsize;j++)
          somme +=gaussien[fsize * i + j];
    for(i = 0; i < fsize; i++)
       for(j = 0; j < fsize; j++)
       {
          convfilter[4 * (fsize * i + j)    ] =
              gaussien[(fsize * i + j)] / somme;
          convfilter[4 * (fsize * i + j) + 1] =
              gaussien[(fsize * i + j)] / somme;
          convfilter[4 * (fsize * i + j) + 2] =
              gaussien[(fsize * i + j)] / somme;
          convfilter[4 * (fsize * i + j) + 3] =
              gaussien[(fsize * i + j)] / somme;
       }

    free(gaussien);

    s->base.privates[nd->screenPrivateIndex].ptr = ns;

    return TRUE;
}


static void
basicblurFiniScreen (CompPlugin *p, CompScreen *s)
{
    BASICBLUR_SCREEN (s);
    freeWindowPrivateIndex (s, ns->windowPrivateIndex);
    UNWRAP (ns, s, drawWindowTexture);
    UNWRAP (ns, s, damageWindowRect);
    UNWRAP (ns, s, paintWindow);
    free (ns);
}

static Bool
basicblurInitWindow (CompPlugin *p, CompWindow *w)
{
    basicblurWindow *nw;

    BASICBLUR_SCREEN (w->screen);

    nw = malloc (sizeof (basicblurWindow));
    if (!nw)
	return FALSE;

    nw->isBlured = FALSE;
    nw->first = FALSE;

    w->base.privates[ns->windowPrivateIndex].ptr = nw;

    return TRUE;
}

static void
basicblurFiniWindow (CompPlugin *p, CompWindow *w)
{
    BASICBLUR_WINDOW (w);
    free (nw);
}

static Bool
basicblurInit (CompPlugin *p)
{
    BasicBlurdisplayPrivateIndex = allocateDisplayPrivateIndex ();
    if (BasicBlurdisplayPrivateIndex < 0)
	return FALSE;

    return TRUE;
}

static void
basicblurFini (CompPlugin *p)
{
    if (BasicBlurdisplayPrivateIndex >= 0)
	freeDisplayPrivateIndex (BasicBlurdisplayPrivateIndex);
    free(convfilter);
}

static CompBool
basicblurInitObject (CompPlugin *p,
		     CompObject *o)
{
    static InitPluginObjectProc dispTab[] = {
	(InitPluginObjectProc) 0, /* InitCore */
	(InitPluginObjectProc) basicblurInitCore,
	(InitPluginObjectProc) basicblurInitDisplay,
	(InitPluginObjectProc) basicblurInitScreen,
	(InitPluginObjectProc) basicblurInitWindow
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
basicblurFiniObject (CompPlugin *p,
		     CompObject *o)
{
    static FiniPluginObjectProc dispTab[] = {
	(FiniPluginObjectProc) 0, /* FiniCore */
	(FiniPluginObjectProc) basicblurFiniCore,
	(FiniPluginObjectProc) basicblurFiniDisplay,
	(FiniPluginObjectProc) basicblurFiniScreen,
	(FiniPluginObjectProc) basicblurFiniWindow
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}


CompPluginVTable basicblurVTable = {
    "basicblur",
    0,
    basicblurInit,
    basicblurFini,
    basicblurInitObject,
    basicblurFiniObject,
    0,
    0
};

CompPluginVTable *
getCompPluginInfo (void)
{
    return &basicblurVTable;
}
