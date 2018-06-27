#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <animation/animation.h>

#include "animationsim_options.h"

extern AnimEffect AnimEffectFlyIn;
extern AnimEffect AnimEffectBounce;
extern AnimEffect AnimEffectRotateIn;
extern AnimEffect AnimEffectSheet;
extern AnimEffect AnimEffectExpand;
extern AnimEffect AnimEffectExpandPW;
extern AnimEffect AnimEffectFan;

// TODO Update this for each added animation effect! (total: 8)
#define NUM_EFFECTS 8

// This must have the value of the first "effect setting" above
// in AnimAddonScreenOptions
#define NUM_NONEFFECT_OPTIONS AnimationsimOptions::FlyinDirection

#define WIN_X(w) ((w)->x () - (w)->input ().left)
#define WIN_Y(w) ((w)->y () - (w)->input ().top)
#define WIN_W(w) ((w)->width () + (w)->input ().left + (w)->input ().right)
#define WIN_H(w) ((w)->height () + (w)->input ().top + (w)->input ().bottom)

class ExtensionPluginAnimSim : public ExtensionPluginInfo
{
public:
    ExtensionPluginAnimSim (const CompString &name,
			    unsigned int nEffects,
			    AnimEffect *effects,
			    CompOption::Vector *effectOptions,
			    unsigned int firstEffectOptionIndex) :
	ExtensionPluginInfo (name, nEffects, effects, effectOptions,
			     firstEffectOptionIndex) {}
    ~ExtensionPluginAnimSim () {}

    const CompOutput *output () { return mOutput; }

private:
    const CompOutput *mOutput;
};

/// Base class for all polygon- and particle-based animations
class BaseSimAnim :
virtual public Animation
{
public:
    BaseSimAnim (CompWindow *w,
		   WindowEvent curWindowEvent,
		   float duration,
		   const AnimEffect info,
		   const CompRect &icon);
    ~BaseSimAnim () {}
    
protected:
    /// Gets info about the extension plugin that implements this animation.
    ExtensionPluginInfo *getExtensionPluginInfo ();
    
    CompositeScreen *mCScreen;
    GLScreen *mGScreen;
    
};


class AnimSimScreen :
    public PluginClassHandler <AnimSimScreen, CompScreen>,
    public AnimationsimOptions
{

public:
    AnimSimScreen (CompScreen *);
    ~AnimSimScreen ();

protected:
    void initAnimationList ();

    CompOutput &mOutput;
};

class AnimSimWindow :
    public PluginClassHandler<AnimSimWindow, CompWindow>
{
public:
    AnimSimWindow (CompWindow *);
    ~AnimSimWindow ();

protected:
    CompWindow *mWindow;    ///< Window being animated.
    AnimWindow *aWindow;
};

/*
typedef struct _WaveParam
{
    float halfWidth;
    float amp;
    float pos;
} WaveParam;
*/

#define ANIMSIM_SCREEN(s)						      \
    AnimSimScreen *ass = AnimSimScreen::get (s);

#define ANIMSIM_WINDOW(w)						      \
    AnimSimWindow *asw = AnimSimWindow::get (w);

class FlyInAnim : public FadeAnim,
		  virtual public BaseSimAnim,
		  virtual public TransformAnim 
{
    public:
	
	FlyInAnim (CompWindow *w,
		   WindowEvent curWindowEvent,
		   float       duration,
		   const AnimEffect info,
		   const CompRect   &icon) :
	    Animation::Animation (w, curWindowEvent, duration, info, icon),
	    BaseSimAnim::BaseSimAnim (w, curWindowEvent, duration, info, icon),
	    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon),
	    FadeAnim::FadeAnim (w, curWindowEvent, duration, info, icon) {}

    protected:
	void step () { TransformAnim::step (); }
	bool updateBBUsed () { return true; }
	void updateBB (CompOutput &output) {  TransformAnim::updateBB (output); }
	void applyTransform ();

	float getFadeProgress () 
	{ 
	    return progressDecelerate (progressLinear ());
	}
};

class RotateInAnim: public TransformAnim,
		    virtual public BaseSimAnim
{
    public:

	RotateInAnim (CompWindow *w,
		      WindowEvent curWindowEvent,
		      float       duration,
		      const AnimEffect info,
		      const CompRect   &icon) :
	    Animation::Animation (w, curWindowEvent, duration, info, icon),
	    BaseSimAnim::BaseSimAnim (w, curWindowEvent, duration, info, icon),
	    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon) {}

    protected:

	void step () { TransformAnim::step (); }
	bool updateBBUsed () { return true; }
	void updateBB (CompOutput &output) { TransformAnim::updateBB (output); }
	void applyTransform ();
	void prePaintWindow ();
	void postPaintWindow ();

	inline float getProgress ()
	{
	    return progressDecelerate (progressLinear ());
	}
};

class ExpandAnim: public TransformAnim,
		  virtual public BaseSimAnim
{
    public:

	ExpandAnim (CompWindow *w,
		    WindowEvent curWindowEvent,
		    float	duration,
		    const AnimEffect info,
		    const CompRect   &icon) :
		Animation::Animation (w, curWindowEvent, duration, info, icon),
		BaseSimAnim::BaseSimAnim (w, curWindowEvent, duration, info, icon),
		TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon) {}

    protected:

	inline float getProgress ()
	{
	    return progressDecelerate (progressLinear ());
	}

	void applyTransform ();
	bool updateBBUsed () { return true; }
	void updateBB (CompOutput &output) { TransformAnim::updateBB (output); }
};

class ExpandPWAnim: public TransformAnim,
		    virtual public BaseSimAnim
{
    public:

	ExpandPWAnim (CompWindow *w,
		      WindowEvent curWindowEvent,
		      float	  duration,
		      const AnimEffect info,
		      const CompRect   &icon) :
		Animation::Animation (w, curWindowEvent, duration, info, icon),
		BaseSimAnim::BaseSimAnim (w, curWindowEvent, duration, info, icon),
		TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon)
		{
		}

    protected:

	inline float getProgress ()
	{
	    return progressDecelerate (progressLinear ());
	}

	void applyTransform ();
	bool updateBBUsed () { return true; }
	void updateBB (CompOutput &output) { TransformAnim::updateBB (output); }
};

class BounceAnim: public FadeAnim,
		  virtual public TransformAnim,
		  virtual public BaseSimAnim
{
    public:

	BounceAnim (CompWindow *w,
		    WindowEvent curWindowEvent,
		    float	  duration,
		    const AnimEffect info,
		    const CompRect   &icon) :
	    Animation::Animation (w, curWindowEvent, duration, info, icon),
	    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon),
	    BaseSimAnim::BaseSimAnim (w, curWindowEvent, duration, info, icon),
	    FadeAnim::FadeAnim (w, curWindowEvent, duration, info, icon)
	    {
		ANIMSIM_SCREEN (screen);

		bounceCount = ass->optionGetBounceNumber ();
		nBounce = 1;
		targetScale = ass->optionGetBounceMinSize ();
		currentScale = ass->optionGetBounceMaxSize ();
		bounceNeg = false;
		currBounceProgress = 0.0f;
		lastProgressMax = 0.0f;
	    }

    protected:

	void step () { TransformAnim::step (); }
	void updateBB (CompOutput &output) { TransformAnim::updateBB (output); }
	bool updateBBUsed () { return true; }

	void applyTransform ();

	float getProgress ();
	float getFadeProgress ()
	{
	    return progressDecelerate (progressLinear ());
	}

	int bounceCount;
	int nBounce;
	float targetScale;
	float currentScale;
	bool  bounceNeg;
	float currBounceProgress;
	float lastProgressMax;
};

class SheetAnim : public GridAnim,
		  virtual public BaseSimAnim
{
    public:

	SheetAnim (CompWindow *w,
		   WindowEvent curWindowEvent,
		   float       duration,
		   const AnimEffect info,
		   const CompRect   &icon);

	class WaveParam
	{
	    public:
		float halfWidth;
		float amp;
		float pos;
	};

    protected:

	void initGrid ()
	{
	    mGridWidth = 30;
	    mGridHeight = 30;
	}

	void step ();
	void updateBB (CompOutput &output);
	bool updateBBUsed () { return true; }
	bool stepRegionUsed () { return true; }

	int sheetsWaveCount;
	std::vector <WaveParam> sheetsWaves;
};

class PulseSingleAnim : public TransformAnim,
			virtual public FadeAnim,
			virtual public BaseSimAnim
{
    public:
      
	PulseSingleAnim (CompWindow *w,
			 WindowEvent curWindowEvent,
			 float	  duration,
			 const      AnimEffect info,
			 const	  CompRect   &icon) :
	    Animation::Animation (w, curWindowEvent, duration, info, icon),
	    FadeAnim::FadeAnim (w, curWindowEvent, duration, info, icon),
	    BaseSimAnim::BaseSimAnim (w, curWindowEvent, duration, info, icon),
	    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon) {}

	void step () { TransformAnim::step (); }
	void updateBB (CompOutput &output) { TransformAnim::updateBB (output); }
	bool updateBBUsed () { return true; }
	
	float getProgress () { return progressLinear (); }
	float getFadeProgress ();
	
	void applyTransform ();
};

class PulseAnim : public MultiAnim <PulseSingleAnim, 2>
{
    public:
      
	PulseAnim (CompWindow *w,
		   WindowEvent curWindowEvent,
		   float       duration,
		   const       AnimEffect info,
		   const       CompRect &icon) :
	    MultiAnim <PulseSingleAnim, 2>::MultiAnim
		  (w, curWindowEvent, duration, info, icon) {}
		  
};

class FanSingleAnim : public TransformAnim,
		      virtual public FadeAnim,
		      virtual public BaseSimAnim
{
    public:
      
	FanSingleAnim (CompWindow  *w,
		       WindowEvent curWindowEvent,
		       float	   duration,
		       const	   AnimEffect info,
		       const	   CompRect   &icon) :
	    Animation::Animation (w, curWindowEvent, duration, info, icon),
	    FadeAnim::FadeAnim (w, curWindowEvent, duration, info, icon),
	    BaseSimAnim::BaseSimAnim (w, curWindowEvent, duration, info, icon),
	    TransformAnim::TransformAnim (w, curWindowEvent, duration, info, icon) {}

	void step () { TransformAnim::step (); }
	void updateBB (CompOutput &output) { TransformAnim::updateBB (output); }
	bool updateBBUsed () { return true; }
	
	float getProgress () { return progressLinear (); }
	float getFadeProgress ();
	
	void applyTransform ();
};

class FanAnim : public MultiAnim <FanSingleAnim, 6>
{
    public:
      
	FanAnim (CompWindow *w,
		 WindowEvent curWindowEvent,
		 float       duration,
		 const       AnimEffect info,
		 const       CompRect &icon) :
	    MultiAnim <FanSingleAnim, 6>::MultiAnim
		  (w, curWindowEvent, duration, info, icon) {}
};

class AnimSimPluginVTable:
    public CompPlugin::VTableForScreenAndWindow <AnimSimScreen, AnimSimWindow>
{
    public:
	bool init ();
};
