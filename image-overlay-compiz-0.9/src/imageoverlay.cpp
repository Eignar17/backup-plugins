/* Copyright (c) 2011 Sam Spilsbury <smspillaz@gmail.com>
 * Licence: GPLv3
 */

#include "imageoverlay.h"

COMPIZ_PLUGIN_20090315 (imageoverlay, ImageOverlayPluginVTable);

/*
 * ImageOverlayScreen::paintTexture
 *
 * Actually draws the texture on screen.
 *
 * Here we get a matrix provided to us from glPaintOutput and then translate
 * the scene accordingly so that the texture is centered at the right place
 * where we want it to be viewed. We then loop through every rect in the display
 * region and map the edge texture co-ordinates to actual scene co-ordinates and
 * ask gl to rasterize it on screen
 */

void
ImageOverlayScreen::paintTexture (GLMatrix &transform)
{
    switch (optionGetPosition ())
    {
        case ImageoverlayOptions::PositionCenteredOnScreen:
            transform.translate (screen->width () / 2 - mTexSize.width () / 2,
                                 screen->height () / 2 - mTexSize.height () / 2, 0.0f);
            break;
        case ImageoverlayOptions::PositionCenteredOnOutput:
        {
            CompOutput *output;
            /* Get the active window and use the default output device for that window
             * and center relative to that output device */

            output = &screen->outputDevs ().at (screen->findWindow (screen->activeWindow ())->outputDevice ());

            transform.translate (output->width () / 2 - mTexSize.width () / 2,
                                 output->height () / 2 - mTexSize.height () / 2, 0.0f);
        }
            break;
        case ImageoverlayOptions::PositionAbsolute:
            transform.translate (optionGetAbsoluteX (), optionGetAbsoluteY (), 0.0f);
            break;

        default:
            break;
    }

    /* Load display matrices */
    glPushMatrix ();
    glLoadMatrixf (transform.getMatrix ());

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glEnable (GL_BLEND);

    /* Check if texture is actually not zero sized */
    if (mTex.size () && !mTexRegion.isEmpty ())
    {
        foreach (GLTexture *tex, mTex)
        {
            CompRect::vector rect = mTexRegion.rects ();
            unsigned int     numRect = mTexRegion.rects ().size (), pos = 0;

            /* Enable this subtexture */
            tex->enable (GLTexture::Fast);

            glBegin (GL_QUADS);

	    while (numRect--)
	    {
		/* bottom left */
		glTexCoord2f (
		    COMP_TEX_COORD_X (tex->matrix (), 0),
		    COMP_TEX_COORD_Y (tex->matrix (), mTexSize.height ()));
		glVertex2i (rect.at (pos).x1 (), rect.at (pos).y2 ());

		/* bottom right */
		glTexCoord2f (
		    COMP_TEX_COORD_X (tex->matrix (), mTexSize.width ()),
		    COMP_TEX_COORD_Y (tex->matrix (), mTexSize.height ()));
		glVertex2i (rect.at (pos).x2 (), rect.at (pos).y2 ());

		/* top right */
		glTexCoord2f (
		    COMP_TEX_COORD_X (tex->matrix (), mTexSize.width ()),
		    COMP_TEX_COORD_Y (tex->matrix (), 0));
		glVertex2i (rect.at (pos).x2 (), rect.at (pos).y1 ());

		/* top left */
		glTexCoord2f (
		    COMP_TEX_COORD_X (tex->matrix (), 0),
		    COMP_TEX_COORD_Y (tex->matrix (), 0));
		glVertex2i (rect.at (pos).x1 (), rect.at (pos).y1 ());

		pos++;
	    }

	    /* Stop rasterizing and disable texture */
	    glEnd ();
	    tex->disable ();
	}
    }

    glDisable (GL_BLEND);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    glPopMatrix ();
}

/*
 * ImageOverlayScreen::updateTexture
 *
 * Read the image file in the provided option to a texture
 *
 */
void
ImageOverlayScreen::updateTexture (CompString &fname)
{
    CompString pname = "imageoverlay";

    mTex.clear ();
    mTex = GLTexture::readImageToTexture (fname, pname, mTexSize);

    mTexRegion = CompRegion (0, 0, mTexSize.width (), mTexSize.height ());
}

/*
 * ImageOverlayScreen::optionChanged
 *
 * Called whenever the "image" option changes according to
 * compizconfig, we update the texture when this happens
 */

void
ImageOverlayScreen::optionChanged (CompOption                   *option,
                                   ImageoverlayOptions::Options num)
{
    if (num == ImageoverlayOptions::Image)
    {
        CompString s = optionGetImage ();
        updateTexture (s);
    }
}

/*
 * ImageOverlayScreen::glPaintOutput
 *
 * Called whenever the screen repaints, we pass the call
 * chain and allow the screen to redraw underneath of us
 * and then we paint the texture on top
 */
bool
ImageOverlayScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
                                   const GLMatrix &matrix,
                                   const CompRegion &region,
                                   CompOutput *output,
                                   unsigned int mask)
{
    bool          status = gScreen->glPaintOutput (attrib, matrix, region, output, mask);
    GLMatrix      iTransform (matrix);

    iTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

    paintTexture (iTransform);

    return status;
}

/*
 * ImageOverlayScreen::toggle
 *
 * Toggled the overlay on and off, hooks and unhooks the paint
 * output hook, unhooks it when it is not in use to save resources
 * since we aren't painting anything anyways
 */
bool
ImageOverlayScreen::toggle ()
{
    CompRegion dRegion = mTexRegion;

    mActive = !mActive;
    gScreen->glPaintOutputSetEnabled (this, mActive);

    /* Determine the region that needs to be "damaged"
     * (eg invalidated and redrawn)
     */
    switch (optionGetPosition ())
    {
        case ImageoverlayOptions::PositionCenteredOnScreen:
            dRegion.translate (screen->width () / 2 - mTexSize.width () / 2,
                                 screen->height () / 2 - mTexSize.height () / 2);
            break;
        case ImageoverlayOptions::PositionCenteredOnOutput:
        {
            CompOutput *output;
            /* Get the active window and use the default output device for that window
             * and center relative to that output device */

            output = &screen->outputDevs ().at (screen->findWindow (screen->activeWindow ())->outputDevice ());

            dRegion.translate (output->width () / 2 - mTexSize.width () / 2,
                                 output->height () / 2 - mTexSize.height () / 2);
        }
            break;
        case ImageoverlayOptions::PositionAbsolute:
            dRegion.translate (optionGetAbsoluteX (), optionGetAbsoluteY ());
            break;

        default:
            break;
    }

    cScreen->damageRegion (dRegion);

    return true;
}

/* Constructor */
ImageOverlayScreen::ImageOverlayScreen (CompScreen *screen) :
    PluginClassHandler <ImageOverlayScreen, CompScreen> (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    mActive (false)
{
    CompString tname = optionGetImage ();

    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (gScreen, false);

    optionSetToggleKeyInitiate (boost::bind (&ImageOverlayScreen::toggle, this));
    optionSetImageNotify (boost::bind (&ImageOverlayScreen::optionChanged, this, _1, _2));

    updateTexture (tname);
}

ImageOverlayScreen::~ImageOverlayScreen ()
{
    mTex.clear ();
}

bool
ImageOverlayPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
        return false;

    return true;
}
