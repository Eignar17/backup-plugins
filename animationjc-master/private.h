#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <animation/animation.h>
#include <animationjc/animationjc.h>

#include "animationjc_options.h"

extern AnimEffect AnimEffectBlackHole;
extern AnimEffect AnimEffectFlicker;
extern AnimEffect AnimEffectGhost;
extern AnimEffect AnimEffectPopcorn;
extern AnimEffect AnimEffectRaindrop;

#define NUM_EFFECTS 5

// This must have the value of the first "effect setting" above
// in AnimJCScreenOptions
#define NUM_NONEFFECT_OPTIONS 0

class ExtensionPluginAnimJC : public ExtensionPluginInfo
{
public:
    ExtensionPluginAnimJC (const CompString &name,
			      unsigned int nEffects,
			      AnimEffect *effects,
			      CompOption::Vector *effectOptions,
			      unsigned int firstEffectOptionIndex) :
	ExtensionPluginInfo (name, nEffects, effects, effectOptions,
			     firstEffectOptionIndex) {}
    ~ExtensionPluginAnimJC () {}

    const CompOutput *output () { return mOutput; }

private:
    const CompOutput *mOutput;
};

class AnimJCScreen :
    public PluginClassHandler<AnimJCScreen, CompScreen, ANIMATIONADDON_ABI>,
    public AnimationjcOptions
{
public:
    AnimJCScreen (CompScreen *);
    ~AnimJCScreen ();

    int getIntenseTimeStep ();

    void initAnimationList ();

private:
    PrivateAnimJCScreen *priv;
};

class PrivateAnimJCScreen
{
    friend class AnimJCScreen;

public:
    PrivateAnimJCScreen (CompScreen *);
    ~PrivateAnimJCScreen ();

protected:
    CompOutput &mOutput;
};

class AnimJCWindow :
    public PluginClassHandler<AnimJCWindow, CompWindow>
{
public:
    AnimJCWindow (CompWindow *);
    ~AnimJCWindow ();

protected:
    CompWindow *mWindow;    ///< Window being animated.
    AnimWindow *aWindow;
};

/*** BLACK HOLE **************************************************************/

class BlackHoleAnim :
    public GridTransformAnim
{
public:
    BlackHoleAnim (CompWindow *w,
                   WindowEvent curWindowEvent,
                   float duration,
                   const AnimEffect info,
                   const CompRect &icon);

    float getBlackHoleProgress () { return progressLinear (); }

    void initGrid ();
    inline bool using3D () { return false; }
    void step ();
};

/*** RAINDROP ****************************************************************/

class RaindropAnim :
    public GridTransformAnim
{
public:
    RaindropAnim (CompWindow *w,
                  WindowEvent curWindowEvent,
                  float duration,
                  const AnimEffect info,
                  const CompRect &icon);

    void initGrid ();

    inline bool using3D () { return true; }

    void step ();
};

/*** POPCORN *****************************************************************/

class PopcornSingleAnim :
    public TransformAnim
{
public:
    PopcornSingleAnim (CompWindow *w,
                       WindowEvent curWindowEvent,
                       float duration,
                       const AnimEffect info,
                       const CompRect &icon) :
	    Animation::Animation
		    (w, curWindowEvent, duration, info, icon),
	    TransformAnim::TransformAnim
		    (w, curWindowEvent, duration, info, icon)
    {
    }

    float layerProgress (int);

    void applyTransform ();

    void updateAttrib (GLWindowPaintAttrib &);

    void updateBB (CompOutput &output);
    bool updateBBUsed () { return true; }
};

class PopcornAnim :
    public MultiAnim <PopcornSingleAnim, 6>
{
public:
    PopcornAnim (CompWindow *w,
                 WindowEvent curWindowEvent,
                 float duration,
                 const AnimEffect info,
                 const CompRect &icon) :
	    MultiAnim <PopcornSingleAnim, 6>::MultiAnim
		    (w, curWindowEvent, duration, info, icon)
    {
    }
};

/*** GHOST *******************************************************************/

class GhostAnim :
    public GridTransformAnim
{
public:
    GhostAnim (CompWindow *w,
               WindowEvent curWindowEvent,
               float duration,
               const AnimEffect info,
               const CompRect &icon);

    void initGrid ();

    inline bool using3D () { return true; }

    void step ();

    void updateAttrib (GLWindowPaintAttrib &);
};

/*** FLICKER *****************************************************************/

class FlickerSingleAnim :
    public GridTransformAnim
{
public:
    FlickerSingleAnim (CompWindow *w,
                       WindowEvent curWindowEvent,
                       float duration,
                       const AnimEffect info,
                       const CompRect &icon) :
	    Animation::Animation
		    (w, curWindowEvent, duration, info, icon),
	    TransformAnim::TransformAnim
		    (w, curWindowEvent, duration, info, icon),
	    GridTransformAnim::GridTransformAnim
		    (w, curWindowEvent, duration, info, icon)
    {
    }

    void updateAttrib (GLWindowPaintAttrib &);

    void initGrid ();

    void step ();

    bool updateBBUsed () { return true; }
    void updateBB (CompOutput &output) { TransformAnim::updateBB (output); }
};

class FlickerAnim :
    public MultiAnim <FlickerSingleAnim, 5>
{
public:
    FlickerAnim (CompWindow *w,
                 WindowEvent curWindowEvent,
                 float duration,
                 const AnimEffect info,
                 const CompRect &icon) :
	    MultiAnim <FlickerSingleAnim, 5>::MultiAnim
		    (w, curWindowEvent, duration, info, icon)
    {
    }
};
