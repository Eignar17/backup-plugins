/**
 *
 * Compiz switcher utility library header
 *
 * switcher-utils.h
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
 
#ifndef SWITCHERUTIL_H
#define SWITCHERUTIL_H


typedef struct _WindowList {
    int nWindows;
    int windowsSize;
    CompWindow **w;
} WindowList;

typedef struct _WindowTree {
    WindowList *windows;
    int childrenSize;
    struct _WindowTree **children;
} WindowTree;

typedef Bool (*WindowCmpProc)(CompWindow *a, CompWindow *b);
typedef Bool (*WindowFilterProc)(CompWindow *a);

/**
 * WindowList constructor
**/
WindowList *newWindowList (void);

/**
 * WindowList destructor
**/
void destroyWindowList (WindowList *l);

/**
 * Add a window to the window list, reallocing if necessary
 * @param l: @WindowList to add the window to.
 * @param w: @CompWindow to add to the list
 *
 * @return: True on success
**/
Bool addWindowToList (WindowList *l,
			     CompWindow *w);

/**
 * WindowTree constructor
**/
WindowTree *newWindowTree (void);

/**
 * WindowTree destructor
**/
void destroyWindowTree (WindowTree *node);

/**
 * Add a window to the window tree
 * @param node:	The @WindowTree to add the window to
 * @param w: The window to add to the tree
 * @param isSameGroup: Comparison function to determine whether w should be
 *		       a child of an existing window.
 *
 * @return: True on success 
**/
Bool addWindowToTree (WindowTree    *node,
		      CompWindow    *w,
		      WindowCmpProc *isSameGroup);

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
WindowTree *createWindowTree (WindowList *windows,
				     WindowCmpProc isSameGroup);

#endif /* SWITCHERUTIL_H */
