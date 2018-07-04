#include "animationsim.h"

float
fxFanAnimProgress (CompWindow *w)
{
    return getProgress ();
};

void
applyFanTransform (CompWindow *w)
{
    /* Starting angle is as a percentage of whichever fan number we are
     * closest to the center
     */
    
    ANIMSIM_SCREEN (screen);
    
    int num = MultiAnim <FanSingleAnim, 6>::getCurrAnimNumber (mAWindow);
    
    if (num > 2)
	num += 1;
    
    float div = (ass->optionGetFanAngle () * 2) / 6;
    float startAng = -(ass->optionGetFanAngle ()) + (div * num);
    float currAng = getProgress () * startAng;
    float offset = (1 - getProgress ()) * (WIN_H (W) / 2);
    
    if (num > 3)
	num += 1;
    
    if (num > 3)
    {
	matrixTranslate (transform, WIN_X (w) + WIN_W (w) - offset,
			      WIN_Y (w) + WIN_H (w),
			      0.0f);
	mTransform.rotate (currAng, 0.0f, 0.0f, 1.0f);
	matrixTranslate (transform, -(WIN_X (w) + WIN_W (w) - offset),
			      -(WIN_Y (w) + WIN_H (w)),
			      0.0f);
    }
    else
    {
	matrixTranslate (transform, WIN_X (w) + offset,
			      WIN_Y (w) + WIN_H (w),
			      0.0f);
	mTransform.rotate (currAng, 0.0f, 0.0f, 1.0f);
	matrixTranslate (transform, -(WIN_X (w) + offset),
			      -(WIN_Y (w) + WIN_H (w)),
			      0.0f);
    }
}
void
fxFanAnimStep (CompWindow *w, float time)
{
    ANIMSIM_DISPLAY (w->screen->display);
    (*ad->animBaseFunc->defaultAnimStep) (w, time);

    applyExpandTransform (w);
}

void
fxFanUpdateWindowAttrib (CompWindow * w,
			   WindowPaintAttrib * wAttrib)
{
}

void
fxFanUpdateWindowTransform (CompWindow *w,
			      CompTransform *wTransform)
{
    ANIMSIM_WINDOW(w);

    matrixMultiply (wTransform, wTransform, &aw->com->transform);
}

Bool
fxFanInit (CompWindow * w)
{
    ANIMSIM_DISPLAY (w->screen->display);

    return (*ad->animBaseFunc->defaultAnimInit) (w);
}
