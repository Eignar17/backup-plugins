#include <private.h>

#define WIN_X(w) ((w)->x () - (w)->input ().left)
#define WIN_Y(w) ((w)->y () - (w)->input ().top)
#define WIN_W(w) ((w)->width () + (w)->input ().left + (w)->input ().right)
#define WIN_H(w) ((w)->height () + (w)->input ().top + (w)->input ().bottom)

float
PopcornSingleAnim::layerProgress (int layer)
{
    if (layer == 0)
	return 0;

    float tStart = (5. - layer) / 6.;
    float tEnd = tStart + 1./3.;

    float t = progressLinear ();

    if (t < tStart)
	return 0;

    if (t > tEnd)
	return 1;

    return (t - tStart) / (tEnd - tStart);
}

void
PopcornSingleAnim::updateAttrib (GLWindowPaintAttrib &attrib)
{
    int layer = MultiAnim <PopcornSingleAnim, 6>::getCurrAnimNumber (mAWindow);

    attrib.opacity *= 1. - progressLinear ();

    attrib.opacity *= 1. - layerProgress (layer);
}

void
PopcornSingleAnim::applyTransform ()
{
    int layer = MultiAnim <PopcornSingleAnim, 6>::getCurrAnimNumber (mAWindow);

    if (layer == 0) return;

    float p = layerProgress (layer);

    float v = 40.;
    float theta = (54. + 144.*(layer-1)) / 180. * 3.14159265;
    float dx = v * cos (theta) * p;
    float dy = -v * sin (theta) * p;

    mTransform.translate (dx, dy,
	    0.16*p*AnimJCScreen::get (screen)->optionGetPopcornKernelHeight ());
}

void
PopcornSingleAnim::updateBB (CompOutput &output)
{
    TransformAnim::updateBB (output);
}
