/* Copyright (c) 2011 Sam Spilsbury <smspillaz@gmail.com>
 * Licence: GPLv3
 */

#include <core/core.h>
#include <opengl/opengl.h>
#include <composite/composite.h>

#include "imageoverlay_options.h"

class ImageOverlayScreen :
    public PluginClassHandler <ImageOverlayScreen, CompScreen>,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public ImageoverlayOptions
{
    public:

        ImageOverlayScreen (CompScreen *);
        ~ImageOverlayScreen ();

        CompositeScreen *cScreen;
        GLScreen        *gScreen;

    public:

        void paintTexture (GLMatrix &transform);
        void updateTexture (CompString &fname);

        void optionChanged (CompOption                   *option,
                            ImageoverlayOptions::Options num);

        bool glPaintOutput (const GLScreenPaintAttrib &attrib,
                            const GLMatrix            &matrix,
                            const CompRegion          &region,
                            CompOutput                *output,
                            unsigned int              mask);

        bool toggle ();

    private:

        GLTexture::List mTex;
        CompRegion      mTexRegion;
        CompSize        mTexSize;

        bool            mActive;
};

#define IMAGE_OVERLAY_SCREEN(screen)                                          \
    ImageOverlayScreen *ios = ImageOverlayScreen::get (screen);

class ImageOverlayPluginVTable :
    public CompPlugin::VTableForScreen <ImageOverlayScreen>
{
    public:

        bool init ();
};
