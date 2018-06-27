#include "animation-internal.h"

/* TODO: Move all the inlines into util.c */

static inline Bool
isWinVisible(CompWindow *w)
{
    return (!w->destroyed &&
	    !(!w->shaded &&
	      (w->attrib.map_state != IsViewable)));
}

static Bool
otherPluginsActive(AnimScreen *as)
{
    int i;
    for (i = 0; i < NUM_WATCHED_PLUGINS; i++)
	if (as->pluginActive[i])
	    return TRUE;
    return FALSE;
}

static Bool
restackInfoStillGood(CompScreen *s, RestackInfo *restackInfo)
{
    Bool wStartGood = FALSE;
    Bool wEndGood = FALSE;
    Bool wOldAboveGood = FALSE;
    Bool wRestackedGood = FALSE;

    CompWindow *w;
    for (w = s->windows; w; w = w->next)
    {
	if (restackInfo->wStart == w && isWinVisible(w))
	    wStartGood = TRUE;
	if (restackInfo->wEnd == w && isWinVisible(w))
	    wEndGood = TRUE;
	if (restackInfo->wRestacked == w && isWinVisible(w))
	    wRestackedGood = TRUE;
	if (restackInfo->wOldAbove == w && isWinVisible(w))
	    wOldAboveGood = TRUE;
    }
    return (wStartGood && wEndGood && wOldAboveGood && wRestackedGood);
}

// Reset stacking related info
void
resetStackingInfo (CompScreen *s)
{
    CompWindow *w;
    for (w = s->windows; w; w = w->next)
    {
	ANIM_WINDOW (w);

	aw->configureNotified = FALSE;
	if (aw->restackInfo)
	{
	    free (aw->restackInfo);
	    aw->restackInfo = NULL;
	}
    }
}

// MOD

// Returns TRUE if linking wCur to wNext would not result
// in a circular chain being formed.
static Bool
wontCreateCircularChain (CompWindow *wCur, CompWindow *wNext)
{
    ANIM_SCREEN (wCur->screen);
    AnimWindow *awNext = NULL;

    while (wNext)
    {
	if (wNext == wCur) // would form circular chain
	    return FALSE;

	awNext = GET_ANIM_WINDOW (wNext, as);
	if (!awNext)
	    return FALSE;

	wNext = awNext->moreToBePaintedNext;
    }
    return TRUE; 
}

static Bool
relevantForRestack(CompWindow *nw)
{
    if (!((nw->wmType &
	   // these two are to be used as "host" windows
	   // to host the painting of windows being focused
	   // at a stacking order lower than them
	   (CompWindowTypeDockMask | CompWindowTypeSplashMask)) ||
	  nw->wmType == CompWindowTypeNormalMask ||
	  nw->wmType == CompWindowTypeDialogMask ||
	  nw->wmType == CompWindowTypeUtilMask ||
	  nw->wmType == CompWindowTypeUnknownMask))
    {
	return FALSE;
    }
    return isWinVisible(nw);
}

void
handleWindowRestack (CompScreen *s)
{
    CompWindow *w; // TODO: Indent correctly
    ANIM_SCREEN (s);
	/*
	  Handle focusing windows with multiple utility/dialog windows
	  (like gobby), as in this case where gobby was raised with its
	  utility windows:

	  was: C0001B 36000A5 1E0000C 1E0005B 1E00050 3205B63 600003 
	  now: C0001B 36000A5 1E0000C 1E00050 3205B63 1E0005B 600003 

	  was: C0001B 36000A5 1E0000C 1E00050 3205B63 1E0005B 600003 
	  now: C0001B 36000A5 1E0000C 3205B63 1E00050 1E0005B 600003 

	  was: C0001B 36000A5 1E0000C 3205B63 1E00050 1E0005B 600003 
	  now: C0001B 36000A5 3205B63 1E0000C 1E00050 1E0005B 600003 
	*/
	CompWindow *wOldAbove = NULL;
	for (w = s->windows; w; w = w->next)
	{
	    ANIM_WINDOW(w);
	    if (aw->restackInfo)
	    {
		if (aw->com.curWindowEvent != WindowEventNone ||
		    otherPluginsActive(as) ||
		    // Don't animate with stale restack info
		    !restackInfoStillGood(s, aw->restackInfo))
		{
		    continue;
		}
		if (!wOldAbove)
		{
		    // Pick the old above of the bottommost one
		    wOldAbove = aw->restackInfo->wOldAbove;
		}
		else
		{
		    // Use as wOldAbove for every focus fading window
		    // (i.e. the utility/dialog windows of an app.)
		    if (wOldAbove != w)
			aw->restackInfo->wOldAbove = wOldAbove;
		}
	    }
	}
	// do in reverse order so that focus-fading chains are handled
	// properly
	for (w = s->reverseWindows; w; w = w->prev)
	{
	    ANIM_WINDOW(w);
	    if (aw->restackInfo)
	    {
		if (aw->com.curWindowEvent != WindowEventNone ||
		    // Don't initiate focus anim for current dodgers
		    aw->com.curAnimEffect != AnimEffectNone ||
		    // Don't initiate focus anim for windows being passed thru
		    aw->winPassingThrough ||
		    otherPluginsActive(as) ||
		    // Don't animate with stale restack info
		    !restackInfoStillGood(s, aw->restackInfo))
		{
		    free(aw->restackInfo);
		    aw->restackInfo = NULL;
		    continue;
		}

		// Find the first window at a higher stacking order than w
		CompWindow *nw;
		for (nw = w->next; nw; nw = nw->next)
		{
		    if (relevantForRestack(nw))
			break;
		}

		// If w is being lowered, there has to be a window
		// at a higher stacking position than w (like a panel)
		// which this w's copy can be painted before.
		// Otherwise the animation will only show w fading in
		// rather than 2 copies of it cross-fading.
		if (!aw->restackInfo->raised && !nw)
		{
		    // Free unnecessary restackInfo
		    free(aw->restackInfo);
		    aw->restackInfo = NULL;
		    continue;
		}

		// Check if above window is focus-fading too.
		// (like a dialog of an app. window)
		// If so, focus-fade this together with the one above
		// (link to it)
		if (nw)
		{
		    AnimWindow *awNext = GET_ANIM_WINDOW(nw, as);
		    if (awNext && awNext->winThisIsPaintedBefore &&
			wontCreateCircularChain (w, nw))
		    {
			awNext->moreToBePaintedPrev = w;
			aw->moreToBePaintedNext = nw;
			aw->restackInfo->wOldAbove =
			    awNext->winThisIsPaintedBefore;
		    }
		}
		initiateFocusAnimation(w);
	    }
	}

	for (w = s->reverseWindows; w; w = w->prev)
	{
	    ANIM_WINDOW(w);

	    if (!aw->isDodgeSubject)
		continue;
	    Bool dodgersAreOnlySubjects = TRUE;
	    CompWindow *dw;
	    AnimWindow *adw;
	    for (dw = aw->dodgeChainStart; dw; dw = adw->dodgeChainNext)
	    {
		adw = GET_ANIM_WINDOW(dw, as);
		if (!adw)
		    break;
		if (!adw->isDodgeSubject)
		    dodgersAreOnlySubjects = FALSE;
	    }
	    if (dodgersAreOnlySubjects)
		aw->skipPostPrepareScreen = TRUE;
	}
}

void
saveRestackInfo (CompWindow *w, Window *clientListStacking, int n)
{
    CompScreen *s = w->screen;
    ANIM_WINDOW (w);
    ANIM_SCREEN (s);
    aw->configureNotified = TRUE;

    // Find which window is restacked 
    // e.g. here 8507730 was raised:
    // 54526074 8507730 48234499 14680072 6291497
    // 54526074 48234499 14680072 8507730 6291497
    // compare first changed win. of row 1 with last
    // changed win. of row 2, and vica versa
    // the matching one is the restacked one
    CompWindow *wRestacked = 0;
    CompWindow *wStart = 0;
    CompWindow *wEnd = 0;
    CompWindow *wOldAbove = 0;
    CompWindow *wChangeStart = 0;
    CompWindow *wChangeEnd = 0;

    Bool raised = FALSE;
    int changeStart = -1;
    int changeEnd = -1;

    int i;
    for (i = 0; i < n; i++)
    {
	CompWindow *wi =
	    findWindowAtScreen (s, clientListStacking[i]);

	// skip if minimized (prevents flashing problem)
	if (!wi || !isWinVisible(wi))
	    continue;

	// skip if (tabbed and) hidden by Group plugin
	if (wi->state & (CompWindowStateSkipPagerMask |
			 CompWindowStateSkipTaskbarMask))
	    continue;

	if (clientListStacking[i] !=
	    as->lastClientListStacking[i])
	{
	    if (changeStart < 0)
	    {
		changeStart = i;
		wChangeStart = wi; // make use of already found w
	    }
	    else
	    {
		changeEnd = i;
		wChangeEnd = wi;
	    }
	}
	else if (changeStart >= 0) // found some change earlier
	    break;
    }

    // if restacking occurred
    if (changeStart >= 0 && changeEnd >= 0)
    {
	CompWindow *w2;

	// if we have only 2 windows changed, 
	// choose the one clicked on
	Bool preferRaised = FALSE;
	Bool onlyTwo = FALSE;

	if (wChangeEnd &&
	    clientListStacking[changeEnd] ==
	    as->lastClientListStacking[changeStart] &&
	    clientListStacking[changeStart] ==
	    as->lastClientListStacking[changeEnd])
	{
	    // Check if the window coming on top was
	    // configureNotified (clicked on)
	    AnimWindow *aw2 = GET_ANIM_WINDOW(wChangeEnd, as);
	    if (aw2->configureNotified)
	    {
		preferRaised = TRUE;
	    }
	    onlyTwo = TRUE;
	}
	// Clear all configureNotified's
	for (w2 = s->windows; w2; w2 = w2->next)
	{
	    AnimWindow *aw2 = GET_ANIM_WINDOW(w2, as);
	    aw2->configureNotified = FALSE;
	}

	if (preferRaised ||
	    (!onlyTwo &&
	     clientListStacking[changeEnd] ==
	     as->lastClientListStacking[changeStart]))
	{
	    // raised
	    raised = TRUE;
	    wRestacked = wChangeEnd;
	    wStart = wChangeStart;
	    wEnd = wRestacked;
	    wOldAbove = wStart;
	}
	else if (clientListStacking[changeStart] ==
		 as->lastClientListStacking[changeEnd] && // lowered
		 // We don't animate lowering if there is no
		 // window above this window, since this window needs
		 // to be drawn on such a "host" in animPaintWindow
		 // (at least for now).
		 changeEnd < n - 1)
	{
	    wRestacked = wChangeStart;
	    wStart = wRestacked;
	    wEnd = wChangeEnd;
	    wOldAbove = findWindowAtScreen
		(s, as->lastClientListStacking[changeEnd+1]);
	}
	for (; wOldAbove && !isWinVisible(wOldAbove);
	     wOldAbove = wOldAbove->next)
	    ;
    }
    if (wRestacked && wStart && wEnd && wOldAbove)
    {
	AnimWindow *awRestacked = GET_ANIM_WINDOW(wRestacked, as);
	if (awRestacked->created)
	{
	    RestackInfo *restackInfo = calloc(1, sizeof(RestackInfo));
	    if (restackInfo)
	    {
		restackInfo->wRestacked = wRestacked;
		restackInfo->wStart = wStart;
		restackInfo->wEnd = wEnd;
		restackInfo->wOldAbove = wOldAbove;
		restackInfo->raised = raised;

		if (awRestacked->restackInfo)
		    free(awRestacked->restackInfo);

		awRestacked->restackInfo = restackInfo;
		as->aWinWasRestackedJustNow = TRUE;
	    }
	}
    }
}
