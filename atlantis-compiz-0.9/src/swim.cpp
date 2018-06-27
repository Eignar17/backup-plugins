/*
 * Compiz cube atlantis plugin
 *
 * atlantis.c
 *
 * This plugin renders a fish tank inside of the transparent cube,
 * replete with fish, crabs, sand, bubbles, and coral.
 *
 * Copyright : (C) 2007-2008 by David Mikos
 * Email     : infiniteloopcounter@gmail.com
 *
 * Copyright : (C) 2007 by Dennis Kasprzyk
 * E-mail    : onestone@opencompositing.org
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
 */

/*
 * swim.c written by David Mikos.
 */

/*
 * Based on swim.c by Dennis Kasprzyk.
 */

/*
 * In turn based on atlantis xscreensaver http://www.jwz.org/xscreensaver/
 */

/* Copyright (c) E. Lassauge, 1998. */

/*
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * The original code for this mode was written by Mark J. Kilgard
 * as a demo for openGL programming.
 *
 * Porting it to xlock  was possible by comparing the original Mesa's morph3d
 * demo with it's ported version to xlock, so thanks for Marcelo F. Vianna
 * (look at morph3d.c) for his indirect help.
 *
 * Thanks goes also to Brian Paul for making it possible and inexpensive
 * to use OpenGL at home.
 *
 * My e-mail address is lassauge@users.sourceforge.net
 *
 * Eric Lassauge  (May-13-1998)
 *
 */

/**
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

#include "atlantis.h"
#include "atlantis_options.h"
#include <math.h>
#include <float.h>

static float boidsPsi;   /* the psi   angle that a fish should turn when
			    only applying boids. */
static float boidsTheta; /* the theta angle that a fish should turn when
			    only applying boids. */

void
FishTransform (fishRec * fish)
{
    glTranslatef (fish->y, fish->z, fish->x);
    glRotatef (fish->psi + 180, 0.0, 1.0, 0.0);
    glRotatef (fish->theta,     1.0, 0.0, 0.0);
}

void
AtlantisScreen::FishPilot (int index)
{
    fishRec * fish = &(mFish[index]);

    int i, j;

    float speed = fish->speed;
    float x = fish->x;
    float y = fish->y;
    float z = fish->z;

    float maxVel = mSpeedFactor * speed;
    float maxTurnAng = 15 * mSpeedFactor; /* left/right angle (in degrees) */
    float maxTurnTh = 5 * mSpeedFactor; /* up/down angle (in degrees) */

    //float minOppTurnAng[2]= { -maxTurnAng, -maxTurnAng };
    float minOppTurnAng[2]= { 0, 0 };

    float ang; /* angle of the fish so that 90-ang is the turn remaining
		  for each wall */
    float tTemp[mHsize]; /* distance to wall when not changing angle */

    float tempAng = atan2f(y, x);
    float dist = hypotf(x, y);
    float perpDist = (mSideDistance - fish->size/2);

    float t = FLT_MAX;
    int iWall = 0; /* The number of the wall most in the way of the fish */

    float distRem, cosAng;
    float mota; /* MIN opposite turn angle */

    float turnRem[2]; /* right/left */
    float sign[2] = { 1, -1 };
    float top[2], bottom[2];
    float Q1;

    float turnAng = 0, turnTh = 0;
    float prefTurnAng, prefTurnTh;

    fish->psi = fmodf(fish->psi, 360);
    if (fish->psi>180)
	fish->psi-=360;
    else if (fish->psi<-180)
	fish->psi+=360;

    if (mWaterHeight-fish->size/2-z<0 && fish->type!=DOLPHIN)
    { /* dolphins are allowed to jump out top briefly */
	fish->z = mWaterHeight-fish->size/2;
    }
    float bottomTank = getGroundHeight(x, y);

    if (z-bottomTank-fish->size/2<0)
    { /* stop all fish from going too far down */
	fish->z = bottomTank+fish->size/2;
    }

    for (i=0; i < mHsize; i++)
    {
	float cosAng = cosf (fmodf (i * mArcAngle * toRadians - tempAng,
	                            2 * PI));

	tTemp[i] = (perpDist - dist * cosAng) / cosf (
	           fmodf (fish->psi * toRadians - i * mArcAngle * toRadians,
	                  2*PI));

	if (cosAng > 0)
	{
	    float d = dist * cosAng - perpDist;

	    if (d > 0) {
		x -= d * cosf (tempAng) *
		     fabsf (cosf (i * mArcAngle * toRadians));
		y -= d * sinf (tempAng) *
		     fabsf (sinf (i * mArcAngle * toRadians));
		fish->x = x;
		fish->y = y;
		tTemp[i] = 0.1;
		tempAng = atan2f (y, x);
		dist = hypotf (x, y);
	    }
	}

	if ((tTemp[i] < t && tTemp[i] >= 0))
	{
	    t = tTemp[i];
	    iWall = i;
	}
    }

    /* need to consider walls iWall, iWall+1 for clockwise iWall, iWall-1 for anticlockwise.
       want the angle that the fish can turn minimally right/left such that the fish can
       still dodge the wall smoothly in the next frame. */

    /* turnRem should be 90 degrees if heading straight at a wall, etc.
       distRem shoud be positive if inside aquarium and negative if outside. */

    ang = fish->psi - iWall * mArcAngle;

    for (j = 0; j < 2; j++)
    {
	float signAng = sign[j] * ang;
	i = iWall;

	cosAng = cosf (fmodf (iWall * mArcAngle * toRadians - tempAng, 2 * PI));
	distRem = fabsf (perpDist - dist * cosAng);

	turnRem[j] = fmodf(90 - signAng, 360);
	if (turnRem[j] < 0)
	    turnRem[j] += 360;

	top[j] = sinf ((signAng + turnRem[j]) * toRadians) - sinf (signAng * toRadians);
	bottom[j] = 2 * distRem / maxVel + cosf (signAng * toRadians) -
		    cosf ((signAng + turnRem[j]) * toRadians);

	Q1 = 2 * atan2f (top[j], bottom[j]) * toDegrees;
	Q1 = fmodf(Q1, 360);
	if (Q1 < 0)
	    Q1 += 360.0f;

	mota = turnRem[j] - maxTurnAng * (turnRem[j] / Q1 - 1);
	if (turnRem[j] / Q1 <= 1 && turnRem[j] / Q1 >= 0)
	    mota = Q1;

	if (mota > minOppTurnAng[j] && mota < 300)
	    minOppTurnAng[j] = mota;

	signAng -= sign[j] * mArcAngle;
	turnRem[j] = fmodf (90 - signAng, 360);
	if (turnRem[j] < 0)
	    turnRem[j] += 360;

	if (j == 0)
	    i = (iWall + 1) % mHsize;
	else
	    i = (iWall + mHsize - 1) % mHsize;

	cosAng = cosf (fmodf (i * mArcAngle * toRadians - tempAng, 2 * PI));
	distRem = fabsf (perpDist - dist * cosAng);

	top[j] = sinf ((signAng + turnRem[j]) * toRadians) - sinf (signAng * toRadians);
	bottom[j] = 2 * distRem / maxVel + cosf (signAng * toRadians) -
		    cosf ((signAng + turnRem[j]) * toRadians);

	Q1 = 2 * atan2f (top[j], bottom[j]) * toDegrees;
	Q1 = fmodf (Q1, 360);
	if (Q1 < 0)
	    Q1 += 360.0f;

	mota = turnRem[j] - maxTurnAng * (turnRem[j] / Q1 - 1);
	if (turnRem[j] / Q1 <= 1 && turnRem[j] / Q1 >= 0)
	    mota = Q1;

	if (mota > minOppTurnAng[j])
	    minOppTurnAng[j] = mota;
    }

    if (fish->boidsCounter <= 0)
    { /* update the boidsAngles */
	BoidsAngle (index);

	fish->boidsCounter = 2 + NUM_GROUPS / mSpeedFactor;
    }
    fish->boidsCounter--;

    boidsPsi   = fish->boidsPsi;
    boidsTheta = fish->boidsTheta;

    prefTurnTh = fmodf (boidsTheta - fish->theta, 360);
    if (prefTurnTh > 180)
	prefTurnTh -= 360;
    if (prefTurnTh <- 180)
	prefTurnTh += 360;

    if (fish->smoothTurnCounter <= 0)
    {
	float divideAmount;

	prefTurnAng = fmodf(boidsPsi - fish->psi, 360); /* boids turn angle*/
	if (prefTurnAng>180)
	    prefTurnAng-=360;
	if (prefTurnAng<-180)
	    prefTurnAng+=360;

	fish->smoothTurnCounter = 5 + NRAND( (int) (20.0 / mSpeedFactor));
	fish->smoothTurnAmount = turnAng;

	switch (fish->type)
	{
	    case SHARK:
		divideAmount = 5 + 10000 / fish->size +
			       randf (10000 / fish->size + 8);
		break;
	    case DOLPHIN:
		divideAmount = 4 + 10000 / fish->size +
			       randf (10000 / fish->size + 5);
		break;

	    default:
		divideAmount = 1 + 10000 / fish->size +
			       randf(1.2 * 10000 / fish->size + 5);
		break;
	}
	if (divideAmount <= 1)
	    divideAmount = 1;

	fish->smoothTurnCounter = (int) 1 + fabsf (prefTurnAng /
	                          (divideAmount * mSpeedFactor / 2));
	turnAng = prefTurnAng / ((float) fish->smoothTurnCounter);
	fish->smoothTurnAmount = turnAng;

	turnTh = prefTurnTh / ((float) fish->smoothTurnCounter);
	fish->smoothTurnTh = turnTh;
    }
    else
    {
	prefTurnAng = fish->smoothTurnAmount;
	turnAng = prefTurnAng;

	prefTurnAng = fish->smoothTurnTh;
	turnTh = prefTurnTh;

	if (minOppTurnAng[0] > prefTurnAng && -minOppTurnAng[0] < prefTurnAng)
	{
	    if (fabsf ( minOppTurnAng[0] - prefTurnAng) <
		fabsf (-minOppTurnAng[1] - prefTurnAng))
		turnAng = minOppTurnAng[0];
	    else
		turnAng = -minOppTurnAng[1];
	}
    }
    fish->smoothTurnCounter--;

    fish->speed += randf(40) - 20; /* new speed */
    if (fish->speed > 200)
	fish->speed = 200;
    if (fish->speed < 50)
	fish->speed = 50;

    if (turnAng > maxTurnAng)
	fish->psi += maxTurnAng;
    else if (turnAng < -maxTurnAng)
	fish->psi -= maxTurnAng;
    else
	fish->psi += turnAng;

    fish->psi = fmodf (fish->psi, 360);

    if (fish->psi > 180)
	fish->psi -= 360;
    else if (fish->psi <- 180)
	fish->psi += 360;

    if (turnTh > maxTurnTh)
	fish->theta += maxTurnTh;
    else if (turnTh <- maxTurnTh)
	fish->theta -= maxTurnTh;
    else
	fish->theta += turnTh;

    if (fish->theta > 50)
	fish->theta = 50;
    else if (fish->theta <- 50)
	fish->theta =- 50;

    fish->x += maxVel * cosf (fish->psi * toRadians) *
	       cosf (fish->theta * toRadians);
    fish->y += maxVel * sinf (fish->psi * toRadians) *
	       cosf (fish->theta * toRadians);
    fish->z += maxVel * sinf (fish->theta * toRadians);
}

void
AtlantisScreen::BoidsAngle (int i)
{
    float x = mFish[i].x;
    float y = mFish[i].y;
    float z = mFish[i].z;

    float psi = mFish[i].psi;
    float theta = mFish[i].theta;

    int type = mFish[i].type;

    float factor = 5+5*fabsf(symmDistr());
    float randPsi = 10*symmDistr();
    float randTh = 10*symmDistr();

    /* (X,Y,Z) is the boids vector */
    float X = factor * cosf ((psi + randPsi) * toRadians) *
	      cosf ((theta + randTh) * toRadians) / 50000;
    float Y = factor * sinf ((psi + randPsi) * toRadians) *
	      cosf ((theta + randTh) * toRadians) / 50000;
    float Z = factor * sinf ((theta + randTh) * toRadians) / 50000;

    float tempAng = atan2f (y, x);
    float dist = hypotf (x, y);

    float perpDist;
    int j;

    for (j = 0; j < mHsize; j++)
    { /* consider side walls */
	float wTheta = j * mArcAngle*toRadians;

	float cosAng = cosf (fmodf (j * mArcAngle * toRadians - tempAng,
	                            2 * PI));
	perpDist = fabsf (mSideDistance - mFish[i].size / 2 -
	                  dist * cosAng);

	if (perpDist > 50000)
	    continue;

	if (perpDist <= mFish[i].size / 2)
	    perpDist  = mFish[i].size / 2;

	factor = 1 / ((float) mHsize);
	if (perpDist <= mFish[i].size)
	    factor *= mFish[i].size / perpDist;

	X -= factor * cosf (wTheta) / perpDist;
	Y -= factor * sinf (wTheta) / perpDist;
    }

    perpDist = mWaterHeight - z; /* top wall */
    if (perpDist <= mFish[i].size / 2)
	perpDist = mFish[i].size / 2;
    factor = 1;
    if (perpDist <= mFish[i].size)
	factor = mFish[i].size / perpDist;
    Z -= factor / perpDist;

    perpDist = z - getGroundHeight(x, y); /* bottom wall */
    if (perpDist <= mFish[i].size / 2)
	perpDist  = mFish[i].size / 2;
    factor = 1;
    if (perpDist <= mFish[i].size)
	factor = mFish[i].size / perpDist;
    Z += factor / perpDist;

    for (j = 0; j < mNumFish; j++)
    { /* consider other fish */
	if (j != i)
	{
	    factor = 1; /* positive means form group, negative means stay away.
			   the amount is proportional to the relative
			   importance of the pairs of fish.*/
	    if (type < mFish[j].type)
	    {
		if (mFish[j].type <= FISH2)
		    factor =-1; /* fish is coming up against different fish */
		else
		    factor = (float) (type - mFish[j].type) * 3;
		    /* fish is coming against a shark, etc. */
	    }
	    else if (type == mFish[j].type)
	    {
		if (mFish[i].group != mFish[j].group &&
		    !optionGetSchoolSimilarGroups ())
		    factor =-1; /* fish is coming up against different fish */
	    }
	    else
		continue; /* whales are not bothered,
			     sharks are not bothered by fish, etc. */

	    if (optionGetSchoolSimilarGroups ())
	    {
		if ( (type == CHROMIS  && (mFish[j].type == CHROMIS2 ||
					   mFish[j].type == CHROMIS3)) ||
		     (type == CHROMIS2 && (mFish[j].type == CHROMIS  ||
					   mFish[j].type == CHROMIS3)) ||
		     (type == CHROMIS3 && (mFish[j].type == CHROMIS  ||
					   mFish[j].type == CHROMIS2)))
		    factor = 1;
	    }

	    float xt = (mFish[j].x - x);
	    float yt = (mFish[j].y - y);
	    float zt = (mFish[j].z - z);
	    float d = sqrtf (xt * xt + yt * yt + zt * zt);

	    float th = fmodf (atan2f (yt, xt) * toDegrees - psi, 360);
	    if (th > 180)
		th -= 360;
	    if (th <- 180)
		th += 360;

	    if (fabsf (th) < 80 &&
		fabsf (asinf (zt / d) * toDegrees - theta ) < 80)
	    { /* in field of view of fish */

		th = fmodf(mFish[j].psi - psi, 360);
		if (th <- 180)
		    th += 360;
		if (th > 180)
		    th -= 360;

		if (factor>0 && (fabsf (th) > 90 ||
				 fabsf (mFish[j].theta - theta) < 90))
		{
		    /* other friendly fish heading in near opposite direction
		       the idea is to turn to form a group by taking the lead
		       or sneaking behind */

		    if (d > 50000 / 2)
		    { /* varies as distance to power [1,2] after
			 this distance */
			d = powf(d, 1 + (d - 50000 / 2) /
			         (2 * 50000 - 50000 / 2));
		    }

		    factor /= d;
		    X += factor * cosf (mFish[j].psi * toRadians) *
			 cosf (mFish[j].theta * toRadians);
		    Y += factor * sinf (mFish[j].psi * toRadians) *
			 cosf (mFish[j].theta * toRadians);
		    Z += factor * sinf (mFish[j].theta * toRadians);
		}
		else
		{
		    if (d > 50000 / 2)
		    { /* varies as distance to power [1,2] after
			 this distance */
			d = powf (d, 2 + (d - 50000 / 2) /
				  (2 * 50000 - 50000 / 2));
		    }
		    else
		    { /* varies as distance */
			d *= d;
		    }

		    /* note an extra factor of d due to
		       normalizing (xt, yt, zt) */

		    factor /= d;
		    X += factor * xt;
		    Y += factor * yt;
		    Z += factor * zt;
		}
	    }
	}
    }

    mFish[i].boidsPsi = atan2f (Y, X) * toDegrees;
    /* needs fmoding, etc to get into range [-180,180]
       angle not relative to fish angle
       (have to minus mFish.psi when using this angle) */

    if (isnan (mFish[i].boidsPsi))
	mFish[i].boidsPsi = psi; /* precaution */

    mFish[i].boidsTheta = asinf (Z / sqrtf (X * X + Y * Y + Z * Z)) *
			     toDegrees;
    if (isnan (mFish[i].boidsTheta))
	mFish[i].boidsTheta = theta; /* precaution */
}
