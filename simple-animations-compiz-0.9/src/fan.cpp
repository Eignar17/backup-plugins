#include "animationsim.h"

float
FanSingleAnim::getFadeProgress ()
{
    return getProgress ();
};

void
FanSingleAnim::applyTransform ()
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
    float offset = (1 - getProgress ()) * (WIN_H (mWindow) / 2);
    
    if (num > 3)
	num += 1;
    
    if (num > 3)
    {
	mTransform.translate (WIN_X (mWindow) + WIN_W (mWindow) - offset,
			      WIN_Y (mWindow) + WIN_H (mWindow),
			      0.0f);
	mTransform.rotate (currAng, 0.0f, 0.0f, 1.0f);
	mTransform.translate (-(WIN_X (mWindow) + WIN_W (mWindow) - offset),
			      -(WIN_Y (mWindow) + WIN_H (mWindow)),
			      0.0f);
    }
    else
    {
	mTransform.translate (WIN_X (mWindow) + offset,
			      WIN_Y (mWindow) + WIN_H (mWindow),
			      0.0f);
	mTransform.rotate (currAng, 0.0f, 0.0f, 1.0f);
	mTransform.translate (-(WIN_X (mWindow) + offset),
			      -(WIN_Y (mWindow) + WIN_H (mWindow)),
			      0.0f);
    }
}
