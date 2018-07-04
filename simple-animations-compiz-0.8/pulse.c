#include "animationsim.h"

/* Keep the "principal" window at 100% opacity, only fade out
 * the window that is "pulsing" away
 */

float
fxPulseAnimProgress (CompWindow *w)
{
    int num = MultiAnim <PulseSingleAnim, 2>::getCurrAnimNumber (mAWindow);
    
    if (num == 1)
	return 1 - fxPulseAnimProgress (w);
    else
	return 0.0f;
};
static void
applyPulseTransform (CompWindow *w)
{
    ANIMSIM_WINDOW (w);

    float scale = 1.0f + (1- fxPulseAnimProgress (w);
    
    /* Add a bit of a "kick" for open, close,
     * minimize, unminimize, etc anims */
    
    switch (CurWindowEvent)
    {
	case WindowEventOpen:
	case WindowEventClose:
	case WindowEventMinimize:
	case WindowEventUnminimize:
	    scale -= 0.2f;
	default:
	    break;
    }
    
    if (MultiAnim <PulseSingleAnim, 2>::getCurrAnimNumber (mAWindow) == 0)
	if (scale > 1.0f)
	    scale = 1.0f;
  
    CompTransform *transform = &aw->com->transform;

    matrixTranslate (transform, WIN_X (w) + WIN_W (w) / 2.0f,
			  WIN_Y (w) + WIN_H (w) / 2.0f, 0.0f);

    matrixScale (transform, scale, scale, 1.0f);

    matrixTranslate (transform, -(WIN_X (w) + WIN_W (w) / 2.0f),
			  -(WIN_Y (w) + WIN_H (w) / 2.0f), 0.0f);
}
void
fxPulseAnimStep (CompWindow *w, float time)
{
    ANIMSIM_DISPLAY (w->screen->display);
    (*ad->animBaseFunc->defaultAnimStep) (w, time);

    applyExpandTransform (w);
}

void
fxPulseUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib * wAttrib)
{
}

void
fxPulseUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform)
{
    ANIMSIM_WINDOW(w);

    matrixMultiply (wTransform, wTransform, &aw->com->transform);
}

Bool
fxPulseInit (CompWindow * w)
{
    ANIMSIM_DISPLAY (w->screen->display);

    return (*ad->animBaseFunc->defaultAnimInit) (w);
}
