/**
 *
 * Compiz switcher utility library
 *
 * switcher-utils.c
 *
 * Copyright © 2007 Christopher James Halse Rogers <chalserogers@gmail.com>
 *
 * Authors:
 * Christopher James Halse Rogers <chalserogers@gmail.com>
 *
 * Based on switcher.c:
 * Copyright : (C) 2007 David Reveman
 * E-mail    : davidr@novell.com
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
 **/

#include <stdlib.h>
#include <string.h>

#include <compiz.h>

#include "switcher-util.h"


/**
 * WindowList constructor
**/
WindowList *
newWindowList (void)
{
    WindowList *ret;
    ret = malloc (sizeof(WindowList));
    ret->nWindows = 0;
    ret->windowsSize = 32;
    ret->w = malloc (sizeof(CompWindow *) * 32);
    return ret;
}

/**
 * WindowList destructor
**/
void
destroyWindowList (WindowList *l)
{
    free (l->w);
    free (l);
}

/**
 * Add a window to the window list, reallocing if necessary
 * @param l: @WindowList to add the window to.
 * @param w: @CompWindow to add to the list
 *
 * @return: True on success
**/
Bool
addWindowToList (WindowList *l,
		 CompWindow *w)
{
    if (l->windowsSize <= l->nWindows)
    {
	l->w = realloc (l->w,
			sizeof (CompWindow *) * (l->nWindows + 32));
	if (!l->w)
	    return FALSE;

	l->windowsSize = l->nWindows + 32;
    }

    l->w[l->nWindows++] = w;
    return TRUE;
}

/**
 * WindowTree constructor
**/
WindowTree *
newWindowTree (void)
{
    WindowTree *ret;
    ret = malloc (sizeof(WindowTree));
    ret->windows = newWindowList();
    ret->childrenSize = 4;
    ret->children = malloc (sizeof (WindowTree *) * 4);
    memset(ret->children, 0, sizeof (WindowTree *) * 4);
    return ret;
}

/**
 * WindowTree destructor
**/
void
destroyWindowTree (WindowTree *node)
{
    for (int i = 0; i < node->windows->nWindows; ++i)
    {
	destroyWindowTree (node->children[i]);
    }
    destroyWindowList (node->windows);
    free (node->children);
    free (node);
}

/**
 * Add a window to the window tree
 * @param node:	The @WindowTree to add the window to
 * @param w: The window to add to the tree
 * @param isSameGroup: Comparison function to determine whether w should be
 *		       a child of an existing window.
 *
 * @return: True on success 
**/
Bool
addWindowToTree (WindowTree    *node,
		 CompWindow    *w,
		 WindowCmpProc *isSameGroup)
{
    if (*isSameGroup)
    {
	for (int i = 0; i < node->windows->nWindows; ++i)
	{
	    if ((**isSameGroup) (node->windows->w[i], w))
	    {
		if (!node->children[i])
		{
		    node->children[i] = newWindowTree ();
		    if (!node->children[i])
			return FALSE;
		}
		return addWindowToTree (node->children[i], w, isSameGroup + 1);
	    }
	}
    }
    if (node->childrenSize <= node->windows->nWindows)
    {
	node->children = realloc (node->children, sizeof (WindowTree *) * \
				  (node->childrenSize + 32));
	if (!node->children)
	    return FALSE;
	memset (node->children + node->childrenSize, 0, 
	       sizeof (WindowTree *) * 32);
	node->childrenSize += 32;
    }
    return addWindowToList(node->windows, w);
}

/**
 * Create a WindowTree from a list of windows.
 *
 * @param windows: The @WindowList to built the tree from.
 * @param isSameGroup: Comparison function, true when the two window should be
 * 		       placed in the same group
 *
 * @return: The root node of the tree.  The caller is responsible for freeing
 *	    the tree once it is no longer needed.
**/
WindowTree *
createWindowTree (WindowList *windows,
		  WindowCmpProc isSameGroup)
{
    WindowTree *root;

    root = newWindowTree();
    	/* XXX: Most naieve possible algorithm! */
    for (int i = 0; i < windows->nWindows; ++i)
    {
	if (!addWindowToTree (root, windows->w[i], isSameGroup))
	{
	    destroyWindowTree(root);
	    return NULL;
	}
    }
    return root;
}
