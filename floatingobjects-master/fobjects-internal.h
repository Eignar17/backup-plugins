#include <math.h>

#include <compiz-core.h>
#include "fobjects_options.h"

#define GET_SNOW_DISPLAY(d)                            \
    ((FObjectsDisplay *) (d)->base.privates[displayPrivateIndex].ptr)

#define SNOW_DISPLAY(d)                                \
    FObjectsDisplay *sd = GET_SNOW_DISPLAY (d)

#define GET_SNOW_SCREEN(s, sd)                         \
    ((FObjectsScreen *) (s)->base.privates[(sd)->screenPrivateIndex].ptr)

#define SNOW_SCREEN(s)                                 \
    FObjectsScreen *ss = GET_SNOW_SCREEN (s, GET_SNOW_DISPLAY (s->display))

static int displayPrivateIndex = 0;

/* -------------------  STRUCTS ----------------------------- */
typedef struct _FObjectsDisplay
{
    int screenPrivateIndex;

    Bool useTextures;

    int             fobjectsTexNFiles;
    CompOptionValue *fobjectsTexFiles;
} FObjectsDisplay;

typedef struct _FObjectsTexture
{
    CompTexture tex;

    unsigned int width;
    unsigned int height;

    Bool   loaded;
    GLuint dList;
} FObjectsTexture;

typedef struct _FObjectsObject
{
    float x, y, z;
    float xs, ys, zs;
    float ra; /* rotation angle */
    float rs; /* rotation speed */

    FObjectsTexture *tex;
} FObjectsObject;

typedef struct _FObjectsScreen
{
    CompScreen *s;

    Bool active;

    CompTimeoutHandle timeoutHandle;

    PaintOutputProc paintOutput;
    DrawWindowProc  drawWindow;

    FObjectsTexture *fobjectsTex;
    int         fobjectsTexturesLoaded;

    GLuint displayList;
    Bool   displayListNeedsUpdate;

    FObjectsObject *allObjects;
} FObjectsScreen;

/* some forward declarations */
static void initiateFObjectsObject (FObjectsScreen * ss, FObjectsObject * sf);
static void fobjectsMove (CompDisplay *d, FObjectsObject * sf);
