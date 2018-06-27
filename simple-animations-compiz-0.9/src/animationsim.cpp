/**
 * Example Animation extension plugin for compiz
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#include "animationsim.h"

COMPIZ_PLUGIN_20090315 (animationsim, AnimSimPluginVTable);

AnimEffect animEffects[NUM_EFFECTS];

ExtensionPluginAnimSim animSimExtPluginInfo (CompString ("animationsim"),
					     NUM_EFFECTS, animEffects, NULL,
                                             NUM_NONEFFECT_OPTIONS);

ExtensionPluginInfo *
BaseSimAnim::getExtensionPluginInfo ()
{
    return &animSimExtPluginInfo;
}

BaseSimAnim::BaseSimAnim (CompWindow *w,
			  WindowEvent curWindowEvent,
			  float duration,
			  const AnimEffect info,
			  const CompRect &icon) :
    Animation::Animation (w, curWindowEvent, duration, info, icon),
    mCScreen (CompositeScreen::get (::screen)),
    mGScreen (GLScreen::get (::screen))
{
}

AnimEffect AnimEffectFlyIn;
AnimEffect AnimEffectRotateIn;
AnimEffect AnimEffectExpand;
AnimEffect AnimEffectExpandPW;
AnimEffect AnimEffectBounce;
AnimEffect AnimEffectSheet;
AnimEffect AnimEffectPulse;
AnimEffect AnimEffectFan;

void
AnimSimScreen::initAnimationList ()
{
    int i = 0;

    animEffects[i++] = AnimEffectFlyIn =
	new AnimEffectInfo ("animationsim:Fly In",
			    true, true, true, false, false,
			    &createAnimation<FlyInAnim>);

    animEffects[i++] = AnimEffectRotateIn =
	new AnimEffectInfo ("animationsim:Rotate In",
			    true, true, true, false, false,
			    &createAnimation<RotateInAnim>);

    animEffects[i++] = AnimEffectExpand =
	new AnimEffectInfo ("animationsim:Expand",
			    true, true, true, false, false,
			    &createAnimation<ExpandAnim>);

    animEffects[i++] = AnimEffectExpandPW =
	new AnimEffectInfo ("animationsim:Expand Piecewise",
			    true, true, true, false, false,
			    &createAnimation<ExpandPWAnim>);

    animEffects[i++] = AnimEffectBounce =
	new AnimEffectInfo ("animationsim:Bounce",
			    true, true, true, false, false,
			    &createAnimation<BounceAnim>);

    animEffects[i++] = AnimEffectSheet =
	new AnimEffectInfo ("animationsim:Sheet",
			    true, true, true, false, false,
			    &createAnimation<SheetAnim>);
    animEffects[i++] = AnimEffectPulse =
	new AnimEffectInfo ("animationsim:Pulse",
			    true, true, true, false, false,
			    &createAnimation<PulseAnim>);
    animEffects[i++] = AnimEffectFan =
	new AnimEffectInfo ("animationsim:Fan",
			    true, true, true, false, false,
			    &createAnimation<FanAnim>);

    animSimExtPluginInfo.effectOptions = &getOptions ();

    AnimScreen *as = AnimScreen::get (::screen);

    // Extends animation plugin with this set of animation effects.
    as->addExtension (&animSimExtPluginInfo);
}

AnimSimScreen::AnimSimScreen (CompScreen *s) :
    //cScreen (CompositeScreen::get (s)),
    //gScreen (GLScreen::get (s)),
    //aScreen (as),
    PluginClassHandler <AnimSimScreen, CompScreen> (s),
    mOutput (s->fullscreenOutput ())
{
    initAnimationList ();
}

AnimSimScreen::~AnimSimScreen ()
{
    AnimScreen *as = AnimScreen::get (::screen);

    as->removeExtension (&animSimExtPluginInfo);

    for (int i = 0; i < NUM_EFFECTS; i++)
    {
	delete animEffects[i];
	animEffects[i] = NULL;
    }
}

AnimSimWindow::AnimSimWindow (CompWindow *w) :
    PluginClassHandler<AnimSimWindow, CompWindow> (w),
    mWindow (w),
    aWindow (AnimWindow::get (w))
{
}

AnimSimWindow::~AnimSimWindow ()
{
    Animation *curAnim = aWindow->curAnimation ();

    if (!curAnim)
	return;

    // We need to interrupt and clean up the animation currently being played
    // by animationsim for this window (if any)
    if (curAnim->remainingTime () > 0 &&
	curAnim->getExtensionPluginInfo ()->name ==
	    CompString ("animationsim"))
    {
	aWindow->postAnimationCleanUp ();
    }
}

bool
AnimSimPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) |
        !CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) |
        !CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI) |
        !CompPlugin::checkPluginABI ("animation", ANIMATION_ABI))
	 return false;

    return true;
}
