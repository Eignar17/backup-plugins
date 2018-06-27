/*
 * Compiz desktopclick plugin
 *
 * desktopclick.cpp
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
 *
 * Based on code by vpswitch, thanks to:
 * Copyright (c) 2007 Dennis Kasprzyk <onestone@opencompositing.org>
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
 * TODO: cleanup indentation
 */

#include "dclick.h"

COMPIZ_PLUGIN_20090315 (dclick, DClickPluginVTable);

/* Option Code Handling  --------------------------------------------------- */

int 
DClickScreen::dclickModMaskFromEnum (int num)
{
    unsigned int mask = 0;

    switch (num)
    {
	case ClickModsControl:
	    mask |= ControlMask;
	    break;
	case ClickModsAlt:
	    mask |= CompAltMask;
	    break;
	case ClickModsShift:
	    mask |= ShiftMask;
	    break;
	case ClickModsSuper:
	    mask |= CompMetaMask;
	    break;
	case ClickModsControlAlt:
	    mask |= ControlMask;
	    mask |= CompAltMask;
	    break;
	case ClickModsControlShift:
	    mask |= ControlMask;
	    mask |= ShiftMask;
	    break;
	case ClickModsControlSuper:
	    mask |= ControlMask;
	    mask |= CompMetaMask;
	    break;
	case ClickModsAltShift:
	    mask |= ShiftMask;
	    mask |= CompAltMask;
	    break;
	case ClickModsAltSuper:
	    mask |= ShiftMask;
	    mask |= CompMetaMask;
	    break;
	case ClickModsShiftSuper:
	    mask |= ShiftMask;
	    mask |= CompMetaMask;
	    break;
	default:
	    break;
    }

    return mask;

}

static int
dclickButtonFromEnum (int button)
{
    return button + 1;
}

/* Inter-plugin communication --------------------------------------- */

static Bool
dclickSendInfoToPlugin (CompOption::Vector arguments, const char *pluginName, const char *actionName, Bool initiate)
{
	Bool pluginExists = FALSE;
	CompOption::Vector options;
	CompOption *option;
	CompPlugin *p;

	p = CompPlugin::find (pluginName);

	if (!p)
	{
		compLogMessage ("desktopclick", CompLogLevelError,
		"Reporting plugin '%s' does not exist!", pluginName);
		return FALSE;
	}

	if (p)
	{
		options = p->vTable->getOptions ();
		option = CompOption::findOption (options, actionName);
		pluginExists = TRUE;
	}

	if (pluginExists)
	{

		if (initiate)
		{

		if (option)
		{
			option->value ().action ().initiate () (&option->value ().action (),
							       0,
		                           		       arguments);
		}
		else
		{
			compLogMessage ("desktopclick", CompLogLevelError,
			"Plugin '%s' does exist, but no option named '%s' was found!", pluginName, actionName);
			return FALSE;
		}
		}
		else
		{
		if (option)
		{
			option->value ().action ().terminate () (&option->value ().action (),
							         0,
		                           		         arguments);
		}
		else
		{
			compLogMessage ("desktopclick", CompLogLevelError,
			"Plugin '%s' does exist, but no option named '%s' was found!", pluginName, actionName);
			return FALSE;
		}
		}
	}

	return true;
}

/* Event Handlers ------------------------------------------------ */

bool
DClickScreen::dclickHandleDesktopButtonPress (int buttonMask,
					      unsigned int win,
					      unsigned int root)
{	

	CompWindow *w = screen->findWindow (win);
	if ((!w || (w->type () & CompWindowTypeDesktopMask) == 0) && 
	(win != screen->root ())) 
	return false; 	/* Oops, we didn't hit the desktop window */

	CompOption::Value::Vector &cModsEnum = optionGetClickMods ();
	CompOption::Value::Vector &cButtonEnum = optionGetClickButtons ();
	CompOption::Value::Vector &cPluginName = optionGetPluginNames ();
	CompOption::Value::Vector &cActionName = optionGetActionNames ();
	
	int nInfo;
	int iInfo = 0;

	if ((cModsEnum.size () != cButtonEnum.size ()) ||
            (cModsEnum.size () != cPluginName.size ())  ||
            (cModsEnum.size () != cActionName.size ()))
	{
	/* Options have not been set correctly */
	return FALSE;
	}

	nInfo = cModsEnum.size ();
	iInfo = 0;

	/* Basically, here we check to see if the button clicked and the mods == the option, and if so call it*/

	while (nInfo-- && iInfo < MAX_INFO)
	{

			if ((buttonMask & dclickButtonFromEnum (cButtonEnum[iInfo].i ())))
			{

				if ((cModsEnum[iInfo].i () != ClickModsNone) && !(activeMods & dclickModMaskFromEnum(cModsEnum[iInfo].i ())))
					return FALSE; /* Mods not pressed */

				CompOption::Vector arguments (0);
				arguments.push_back (CompOption ("window", CompOption::TypeInt));
				arguments.push_back (CompOption ("root", CompOption::TypeInt));

				arguments[0].value ().set ((int) win);
				arguments[1].value ().set ((int) root);

				/* Ensure that the strings are there before we do anything stupid */
				if (cPluginName[iInfo].s ().c_str () && cActionName[iInfo].s ().c_str ())
				{
				dclickSendInfoToPlugin (arguments, cPluginName[iInfo].s ().c_str (), cActionName[iInfo].s ().c_str (), TRUE);
				}
		      }

			iInfo++;

	}

	return true;
}


bool
DClickScreen::dclickHandleDesktopButtonRelease (int buttonMask,
				   		unsigned int win,
				   		unsigned int root)
{
	CompOption::Value::Vector &cModsEnum = optionGetClickMods ();
	CompOption::Value::Vector &cButtonEnum = optionGetClickButtons ();
	CompOption::Value::Vector &cPluginName = optionGetPluginNames ();
	CompOption::Value::Vector &cActionName = optionGetActionNames ();
	CompOption::Value::Vector &cCallType   = optionGetCallType ();
	
	int nInfo;
	int iInfo = 0;

	if ((cModsEnum.size () != cButtonEnum.size ()) ||
            (cModsEnum.size () != cPluginName.size ())  ||
            (cModsEnum.size () != cActionName.size ()) ||
	    (cModsEnum.size () != cCallType.size ()))
	{
	/* Options have not been set correctly */
	return false;
	}

	nInfo = cModsEnum.size ();
	iInfo = 0;

	/* Basically, here we check to see if the button clicked and the mods == the option, and if so call it*/

	while (nInfo-- && iInfo < MAX_INFO)
	{

			if (cCallType[iInfo].i () != CallTypeToggle)
			{
				CompOption::Vector arguments (0);
				arguments.push_back (CompOption ("window", CompOption::TypeInt));
				arguments.push_back (CompOption ("root", CompOption::TypeInt));

				arguments[0].value ().set ((int) win);
				arguments[1].value ().set ((int) root);

				/* Ensure that the strings are there before we do anything stupid */
				if (cPluginName[iInfo].s ().c_str () && cActionName[iInfo].s ().c_str ())
				{
				dclickSendInfoToPlugin (arguments, cPluginName[iInfo].s ().c_str (), cActionName[iInfo].s ().c_str (), FALSE);
				}
		      }

			iInfo++;

	}

	return true;

}


void
DClickScreen::handleEvent (XEvent      *event)
{

    switch (event->type)
    {

    case ButtonPress:
	dclickHandleDesktopButtonPress(event->xbutton.button, event->xbutton.window, event->xbutton.root);
	break;
    case ButtonRelease:
	dclickHandleDesktopButtonRelease(event->xbutton.button, event->xbutton.window, event->xbutton.root);
	break;
    default:
	break;    
    }

    if (event->type == screen->xkbEvent ())
    {
	XkbAnyEvent *xkbEvent = (XkbAnyEvent *) event;

	if (xkbEvent->xkb_type == XkbStateNotify)
	{
	    XkbStateNotifyEvent *stateEvent = (XkbStateNotifyEvent *) event;
	    activeMods = stateEvent->mods;
	}
    }
    screen->handleEvent (event);
}

DClickScreen::DClickScreen (CompScreen *s) :
    PluginClassHandler <DClickScreen, CompScreen> (screen),
    activeMods (0xFFFFFF)
{
    ScreenInterface::setHandler (screen);
}

bool DClickPluginVTable::init ()
{

    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    return true;
}

