#include "private.h"

GhostAnim::GhostAnim (CompWindow *w,
		      WindowEvent curWindowEvent,
		      float duration,
		      const AnimEffect info,
		      const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon),
    GridTransformAnim::GridTransformAnim (w, curWindowEvent, duration, info,
                                          icon)
{
}

void
GhostAnim::updateAttrib (GLWindowPaintAttrib &attrib)
{
    AnimJCScreen *ajs = AnimJCScreen::get (screen);
    attrib.opacity *= 1. - progressLinear ();
    attrib.saturation *= ajs->optionGetGhostSaturation ();
}

void
GhostAnim::initGrid ()
{
    AnimJCScreen *ajs = AnimJCScreen::get (screen);
    mGridWidth = ajs->optionGetGhostGrid ();
    mGridHeight = ajs->optionGetGhostGrid ();
}

void
GhostAnim::step ()
{
    float t = 1. - progressLinear ();
    if (mCurWindowEvent == WindowEventClose)
	t = 1. - t;

    CompRect winRect (mAWindow->savedRectsValid () ?
                      mAWindow->saveWinRect () :
                      mWindow->geometry ());
    CompRect outRect (mAWindow->savedRectsValid () ?
                      mAWindow->savedOutRect () :
                      mWindow->outputRect ());
    CompWindowExtents outExtents (mAWindow->savedRectsValid () ?
			          mAWindow->savedOutExtents () :
			          mWindow->output ());

    int wx = winRect.x ();
    int wy = winRect.y ();

    int owidth = outRect.width ();
    int oheight = outRect.height ();

    AnimJCScreen *ajs = AnimJCScreen::get (screen);

    float waveAmp = 3 * ajs->optionGetGhostAmplitude ();
    float waveLengthX1 = 0.4;
    float waveLengthX2 = 0.3;
    float waveLengthY1 = 0.45;
    float waveLengthY2 = 0.35;
    float wavePositionX1 =  0.25 * t * ajs->optionGetGhostWaveSpeed ();
    float wavePositionX2 = -0.25 * t * ajs->optionGetGhostWaveSpeed ();
    float wavePositionY1 =  0.25 * t * ajs->optionGetGhostWaveSpeed ();
    float wavePositionY2 = -0.25 * t * ajs->optionGetGhostWaveSpeed ();

    GridModel::GridObject *object = mModel->objects ();
    unsigned int n = mModel->numObjects ();
    for (unsigned int i = 0; i < n; i++, object++)
    {
	Point3d &objPos = object->position ();

	float origx = wx + mModel->scale ().x () *
		(owidth * object->gridPosition ().x () -
		outExtents.left);

	float origy = wy + mModel->scale ().y () *
		(oheight * object->gridPosition ().y () -
		outExtents.top);

	float x = object->gridPosition ().x ();
	float y = object->gridPosition ().y ();

	float distFromWaveX1 = x - wavePositionX1;
	float distFromWaveX2 = x - wavePositionX2;
	float distFromWaveY1 = y - wavePositionY1;
	float distFromWaveY2 = y - wavePositionY2;

	objPos.setX (origx +
		     waveAmp * sin (distFromWaveX1 / waveLengthX1 * 2 * M_PI) +
		     waveAmp * sin (distFromWaveX2 / waveLengthX2 * 2 * M_PI));

	objPos.setY (origy +
		     waveAmp * sin (distFromWaveY1 / waveLengthY1 * 2 * M_PI) +
		     waveAmp * sin (distFromWaveY2 / waveLengthY2 * 2 * M_PI));

	objPos.setZ (0);
    }
}
