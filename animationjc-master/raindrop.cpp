#include "private.h"

#include <algorithm>

RaindropAnim::RaindropAnim (CompWindow *w,
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
RaindropAnim::initGrid ()
{
    mGridWidth = 20;
    mGridHeight = 20;
}

void
RaindropAnim::step ()
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

    float waveLength = ajs->optionGetRaindropWavelength ();
    int numWaves = ajs->optionGetRaindropNumWaves ();
    float waveAmp = (pow ((float)oheight / ::screen->height (), 0.4) * 0.08) *
	    ajs->optionGetRaindropAmplitude ();
    float wavePosition = -waveLength * numWaves +
	    (1. + waveLength * numWaves) * t;

    GridModel::GridObject *object = mModel->objects ();
    unsigned int n = mModel->numObjects ();
    for (unsigned int i = 0; i < n; i++, object++)
    {
	Point3d &objPos = object->position ();

        float origx = wx + mModel->scale ().x () *
		(owidth * object->gridPosition ().x () -
		outExtents.left);
        objPos.setX (origx);

	float origy = wy + mModel->scale ().y () *
		(oheight * object->gridPosition ().y () -
		outExtents.top);
        objPos.setY (origy);

	// find distance to center in grid terms
	float gridDistance = sqrt (pow (object->gridPosition ().x ()-0.5, 2) +
			           pow (object->gridPosition ().y ()-0.5, 2)) *
			     sqrt (2);

	float distFromWave = gridDistance - wavePosition;
	if (distFromWave < waveLength*numWaves && distFromWave > 0)
	    objPos.setZ (waveAmp *
		    sin (3.14159265 * distFromWave / waveLength / numWaves) *
		    pow (sin (3.14159265 * distFromWave / waveLength), 2));
	else
	    objPos.setZ (0);
    }
}
