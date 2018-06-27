// Go to the bottommost window in this "focus chain"
// This chain is used to handle some cases: e.g when Find dialog
// of an app is open, both windows should be faded when the Find
// dialog is raised.

/* walker.c */

#include "animation-internal.h"

static CompWindow*
getBottommostInFocusChain (CompWindow *w)
{
    if (!w)
	return w;

    ANIM_WINDOW (w);
    ANIM_SCREEN (w->screen);

    CompWindow *bottommost = aw->winToBePaintedBeforeThis;

    if (!bottommost || bottommost->destroyed)
	return w;

    AnimWindow *awBottommost = GET_ANIM_WINDOW (bottommost, as);
    CompWindow *wPrev = NULL;

    if (awBottommost)
	wPrev = awBottommost->moreToBePaintedPrev;
    while (wPrev)
    {
	bottommost = wPrev;
	wPrev = GET_ANIM_WINDOW(wPrev, as)->moreToBePaintedPrev;
    }
    return bottommost;
}

static void
resetWalkerMarks (CompScreen *s)
{
    CompWindow *w;
    for (w = s->windows; w; w = w->next)
    {
	ANIM_WINDOW(w);
	aw->walkerOverNewCopy = FALSE;
	aw->walkerVisitCount = 0;
    }
}

static CompWindow*
animWalkFirst (CompScreen *s)
{
    ANIM_SCREEN (s);

    resetWalkerMarks (s);

    CompWindow *w = getBottommostInFocusChain(s->windows);
    if (w)
    {
	AnimWindow *aw = GET_ANIM_WINDOW (w, as);
	aw->walkerVisitCount++;
    }
    return w;
}

static CompWindow*
animWalkLast (CompScreen *s)
{
    ANIM_SCREEN (s);

    resetWalkerMarks (s);

    CompWindow *w = s->reverseWindows;
    if (w)
    {
	AnimWindow *aw = GET_ANIM_WINDOW (w, as);
	aw->walkerVisitCount++;
    }
    return w;
}

static Bool
markNewCopy (CompWindow *w)
{
    ANIM_WINDOW (w);

    // if window is in a focus chain
    if (aw->winThisIsPaintedBefore ||
	aw->moreToBePaintedPrev)
    {
	aw->walkerOverNewCopy = TRUE;
	return TRUE;
    }
    return FALSE;
}

static CompWindow*
animWalkNext (CompWindow *w)
{
    ANIM_WINDOW (w);
    CompWindow *wRet = NULL;

    if (!aw->walkerOverNewCopy)
    {
	// Within a chain? (not the 1st or 2nd window)
	if (aw->moreToBePaintedNext)
	    wRet = aw->moreToBePaintedNext;
	else if (aw->winThisIsPaintedBefore) // 2nd one in chain?
	    wRet = aw->winThisIsPaintedBefore;
    }
    else
	aw->walkerOverNewCopy = FALSE;

    if (!wRet && w->next && markNewCopy (w->next))
	wRet = w->next;
    else if (!wRet)
	wRet = getBottommostInFocusChain(w->next);

    if (wRet)
    {
	ANIM_SCREEN (w->screen);

	AnimWindow *awRet = GET_ANIM_WINDOW (wRet, as);
	// Prevent cycles, which cause freezes
	if (awRet->walkerVisitCount > 1) // each window is visited at most twice
	    return NULL;
	awRet->walkerVisitCount++;
    }
    return wRet;
}

static CompWindow*
animWalkPrev (CompWindow *w)
{
    ANIM_WINDOW (w);
    CompWindow *wRet = NULL;

    // Focus chain start?
    CompWindow *w2 = aw->winToBePaintedBeforeThis;
    if (w2)
	wRet = w2;
    else if (!aw->walkerOverNewCopy)
    {
	// Within a focus chain? (not the last window)
	CompWindow *wPrev = aw->moreToBePaintedPrev;
	if (wPrev)
	    wRet = wPrev;
	else if (aw->winThisIsPaintedBefore) // Focus chain end?
	    // go to the chain beginning and get the
	    // prev. in X stacking order
	{
	    if (aw->winThisIsPaintedBefore->prev)
		markNewCopy (aw->winThisIsPaintedBefore->prev);

	    wRet = aw->winThisIsPaintedBefore->prev;
	}
    }
    else
	aw->walkerOverNewCopy = FALSE;

    if (!wRet && w->prev)
	markNewCopy (w->prev);

    wRet = w->prev;
    if (wRet)
    {
	ANIM_SCREEN (w->screen);

	AnimWindow *awRet = GET_ANIM_WINDOW (wRet, as);
	// Prevent cycles, which cause freezes
	if (awRet->walkerVisitCount > 1) // each window is visited at most twice
	    return NULL;
	awRet->walkerVisitCount++;
    }
    return wRet;
}

void
animInitWindowWalker (CompScreen *s,
		      CompWalker *walker)
{
    ANIM_SCREEN (s);

    UNWRAP (as, s, initWindowWalker);
    (*s->initWindowWalker) (s, walker);
    WRAP (as, s, initWindowWalker, animInitWindowWalker);

    if (as->walkerAnimCount > 0) // only walk if necessary
    {
	if (!as->animInProgress) // just in case
	{
	    as->walkerAnimCount = 0;
	    return;
	}
	walker->first = animWalkFirst;
	walker->last  = animWalkLast;
	walker->next  = animWalkNext;
	walker->prev  = animWalkPrev;
    }
}

