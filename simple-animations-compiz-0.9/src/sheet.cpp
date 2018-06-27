/*
 * Animation plugin for compiz/beryl
 *
 * animation.c
 *
 * Copyright : (C) 2006 Erkin Bahceci
 * E-mail    : erkinbah@gmail.com
 *
 * Based on Wobbly and Minimize plugins by
 *           : David Reveman
 * E-mail    : davidr@novell.com>
 *
 * Particle system added by : (C) 2006 Dennis Kasprzyk
 * E-mail                   : onestone@beryl-project.org
 *
 * Beam-Up added by : Florencio Guimaraes
 * E-mail           : florencio@nexcorp.com.br
 *
 * Hexagon tessellator added by : Mike Slegeir
 * E-mail                       : mikeslegeir@mail.utexas.edu>
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
 */

#include "animationsim.h"

SheetAnim::SheetAnim (CompWindow *w,
		      WindowEvent curWindowEvent,
		      float       duration,
		      const AnimEffect info,
		      const CompRect   &minIcon) :
    Animation::Animation (w, curWindowEvent, duration, info, minIcon),
    BaseSimAnim::BaseSimAnim (w, curWindowEvent, duration, info, minIcon),
    GridAnim::GridAnim (w, curWindowEvent, duration, info, minIcon)
{
    int maxWaves;
    float waveAmpMin, waveAmpMax;
    float distance;
    CompWindow *parent;
    CompRect   icon = minIcon;

    foreach (parent, screen->windows ())
    {
	if (parent->transientFor () == w->id () && parent->id () != w->id ())
	    break;
    }

    if (parent)
    {
	icon.setX (WIN_X (parent) + WIN_W (parent) / 2.0f);
	icon.setY (WIN_Y (parent));
	icon.setWidth (WIN_W (w));
    }
    else
    {
	icon.setX (screen->width () / 2.0f);
	icon.setY (0.0f);;
	icon.setWidth (WIN_W (w));
    }

    maxWaves = 0;
    waveAmpMin = 0.0f;
    waveAmpMax = 0.0f;

    if (maxWaves == 0)
    {
	sheetsWaveCount = 0;
    }
    else
    {
	// Initialize waves

	distance = WIN_Y(w) + WIN_H(w) - icon.y ();

	sheetsWaveCount =
	1 + (float)maxWaves *distance;

	if (sheetsWaves.empty ())
	{
	    sheetsWaves.resize (sheetsWaveCount);
	}
	// Compute wave parameters

	int ampDirection = (RAND_FLOAT() < 0.5 ? 1 : -1);
	float minHalfWidth = 0.22f;
	float maxHalfWidth = 0.38f;

	for (unsigned int i = 0; i < sheetsWaves.size (); i++)
	{
	    sheetsWaves[i].amp =
		ampDirection * (waveAmpMax - waveAmpMin) *
		rand() / RAND_MAX + ampDirection * waveAmpMin;
	    sheetsWaves[i].halfWidth =
		RAND_FLOAT() * (maxHalfWidth -
				minHalfWidth) + minHalfWidth;

	    // avoid offset at top and bottom part by added waves
	    float availPos = 1 - 2 * sheetsWaves[i].halfWidth;
	    float posInAvailSegment = 0;

	    if (i > 0)
	        posInAvailSegment =
		    (availPos / sheetsWaveCount) * rand() / RAND_MAX;

	    sheetsWaves[i].pos =
	        (posInAvailSegment +
	         i * availPos / sheetsWaveCount +
	         sheetsWaves[i].halfWidth);

	    // switch wave direction
	    ampDirection *= -1;
	}
    }
}

void
SheetAnim::updateBB (CompOutput &output)
{
    // TODO: Just consider the corner objects

    CompositeScreen::get (screen)->damageScreen (); // XXX: *COUGH!!!!*
}

void
SheetAnim::step ()
{
    GridModel *model = mModel;
    CompRect &icon = mIcon;
    CompWindow *parent;

    foreach (parent, screen->windows ())
    {
	if (parent->transientFor () == mWindow->id () && parent->id () != mWindow->id ())
	    break;
    }

    if (parent)
    {
	icon.setX (WIN_X (parent) + WIN_W (parent) / 2.0f);
	icon.setY (WIN_Y (parent));
	icon.setWidth (WIN_W (mWindow));
    }
    else
    {
	icon.setX (screen->width () / 2.0f);
	icon.setY (0.0f);;
	icon.setWidth (WIN_W (mWindow));
    }

    float forwardProgress = progressLinear ();

    if (sheetsWaveCount > 0 && sheetsWaves.empty ())
	return;

    float iconCloseEndY;
    float iconFarEndY;
    float winFarEndY;
    float winVisibleCloseEndY;
    float winw = WIN_W(mWindow);
    float winh = WIN_H(mWindow);


    iconFarEndY = icon.y ();
    iconCloseEndY = icon.y () + icon.height ();
    winFarEndY = WIN_Y(mWindow) + winh;
    winVisibleCloseEndY = WIN_Y(mWindow);
    if (winVisibleCloseEndY < iconCloseEndY)
	winVisibleCloseEndY = iconCloseEndY;


    float preShapePhaseEnd = 0.22f;
    float preShapeProgress  = 0;
    float postStretchProgress = 0;
    float stretchProgress = 0;
    float stretchPhaseEnd =
	preShapePhaseEnd + (1 - preShapePhaseEnd) *
	(iconCloseEndY -
	 winVisibleCloseEndY) / ((iconCloseEndY - winFarEndY) +
				 (iconCloseEndY - winVisibleCloseEndY));
    if (stretchPhaseEnd < preShapePhaseEnd + 0.1)
	stretchPhaseEnd = preShapePhaseEnd + 0.1;

    if (forwardProgress < preShapePhaseEnd)
    {
	preShapeProgress = forwardProgress / preShapePhaseEnd;

	// Slow down "shaping" toward the end
	preShapeProgress = 1 - progressDecelerate (1 - preShapeProgress);
    }

    if (forwardProgress < preShapePhaseEnd)
    {
	stretchProgress = forwardProgress / stretchPhaseEnd;
    }
    else
    {
	if (forwardProgress < stretchPhaseEnd)
	{
	    stretchProgress = forwardProgress / stretchPhaseEnd;
	}
	else
	{
	    postStretchProgress =
		(forwardProgress - stretchPhaseEnd) / (1 - stretchPhaseEnd);
	}
    }

    GridModel::GridObject *object = mModel->objects ();
    unsigned int i;
    for (i = 0; i < mModel->numObjects (); i++, object++)
    {
	float origx = mWindow->x () + (winw * object->gridPosition ().x () -
				     mWindow->output ().left) * model->scale ().x ();
	float origy = mWindow->y () + (winh * object->gridPosition ().y () -
				     mWindow->output ().top) * model->scale ().y ();
	float icony = icon.y () + icon.height ();

	float stretchedPos;
	Point3d &objPos = object->position ();
	stretchedPos =
		object->gridPosition ().y () * origy +
		(1 - object->gridPosition ().y ()) * icony;

	// Compute current y position
	if (forwardProgress < preShapePhaseEnd)
	{
	    objPos.setY ((1 - stretchProgress) * origy +
		stretchProgress * stretchedPos);
	}
	else
	{
	    if (forwardProgress < stretchPhaseEnd)
	    {
		objPos.setY ((1 - stretchProgress) * origy +
		    stretchProgress * stretchedPos);
	    }
	    else
	    {
		objPos.setY ((1 - postStretchProgress) *
		    stretchedPos +
		    postStretchProgress *
		    (stretchedPos + (iconCloseEndY - winFarEndY)));
	    }
	}

	// Compute "target shape" x position
	float yProgress = (iconCloseEndY - object->position ().y () ) / (iconCloseEndY - winFarEndY);

	float targetx = yProgress * (origx - icon.x ())
	 + icon.x () + icon.width () * (object->gridPosition ().x () - 0.5);

	// Compute current x position
	if (forwardProgress < preShapePhaseEnd)
	    objPos.setX ((1 - preShapeProgress) * origx + preShapeProgress * targetx);
	else
	    objPos.setX (targetx);

	if (object->position ().y () < iconFarEndY)
	    objPos.setY (iconFarEndY);

	// No need to set object->position.z to 0, since they won't be used
	// due to modelAnimIs3D being FALSE for magic lamp.
    }
}

