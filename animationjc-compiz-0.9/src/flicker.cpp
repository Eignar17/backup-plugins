#include <private.h>

void
FlickerSingleAnim::updateAttrib (GLWindowPaintAttrib &attrib)
{
    int layer = MultiAnim <FlickerSingleAnim,5>::getCurrAnimNumber (mAWindow);
    float o = 0.2;
    attrib.opacity *= o / (1. - (4-layer)*o);
}

void
FlickerSingleAnim::initGrid ()
{
    mGridWidth = 2;
    mGridHeight = 20;
}

void
FlickerSingleAnim::step ()
{
    int layer = MultiAnim <FlickerSingleAnim,5>::getCurrAnimNumber (mAWindow);

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

    float t = 1 - progressLinear ();
    if (mCurWindowEvent == WindowEventClose)
	t = 1 - t;

    float amplitude = AnimJCScreen::get (screen)->optionGetFlickerAmplitude ();
    float waveLength = 0.4;
    float wavePosition = -waveLength + (1. + waveLength) * t;

    float displacement;

    GridModel::GridObject *object = mModel->objects ();
    unsigned int n = mModel->numObjects ();
    for (unsigned int i = 0; i < n; i++, object++)
    {
	Point3d &objPos = object->position ();

	if (i % 2 == 0) // left side; reuse old displacement on right side
	{
	    float distFromWave = object->gridPosition ().y () - wavePosition;

	    if (distFromWave > 0 && distFromWave <= waveLength)
	    {
		displacement = amplitude * sin (distFromWave/waveLength * M_PI);
	    }
	    else
	    {
		displacement = 0;
	    }
	}

        float x = wx + mModel->scale ().x () *
            (owidth * object->gridPosition ().x () - outExtents.left);

	float y = wy + mModel->scale ().y () *
	    (oheight * object->gridPosition ().y () - outExtents.top);

	switch (layer)
	{
	    case 1:
		x -= displacement;
		break;
	    case 2:
		x += displacement;
		break;
	    case 3:
		y -= displacement;
		break;
	    case 4:
		y += displacement;
		break;
	    default:
		break;
	}

        objPos.setX (x);
	objPos.setY (y);
	objPos.setZ (0);
    }
}
