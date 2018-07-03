#include "animationsim.h"

/* Keep the "principal" window at 100% opacity, only fade out
 * the window that is "pulsing" away
 */

float
PulseSingleAnim::getFadeProgress ()
{
    int num = MultiAnim <PulseSingleAnim, 2>::getCurrAnimNumber (mAWindow);
    
    if (num == 1)
	return 1 - getProgress ();
    else
	return 0.0f;
};

void
PulseSingleAnim::applyTransform ()
{
    float scale = 1.0f + (1- getProgress ());
    
    /* Add a bit of a "kick" for open, close,
     * minimize, unminimize, etc anims */
    
    switch (mCurWindowEvent)
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
  
    GLMatrix *transform = &mTransform;

    transform->translate (WIN_X (mWindow) + WIN_W (mWindow) / 2.0f,
			  WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f, 0.0f);

    transform->scale (scale, scale, 1.0f);

    transform->translate (-(WIN_X (mWindow) + WIN_W (mWindow) / 2.0f),
			  -(WIN_Y (mWindow) + WIN_H (mWindow) / 2.0f), 0.0f);
}