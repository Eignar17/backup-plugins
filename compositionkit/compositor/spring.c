/*
 * Copyright © 2008 Kristian Høgsberg
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "compositor.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "wayland-server.h"

WL_EXPORT void
wlsc_spring_init(struct wlsc_spring *spring,
		 double k, double current, double target)
{
	spring->k = k;
	spring->friction = 100.0;
	spring->current = current;
	spring->previous = current;
	spring->target = target;
}

WL_EXPORT void
wlsc_spring_update(struct wlsc_spring *spring, uint32_t msec)
{
	double force, v, current, step;

	step = (msec - spring->timestamp) / 300.0;
	spring->timestamp = msec;

	current = spring->current;
	v = current - spring->previous;
	force = spring->k * (spring->target - current) / 10.0 +
		(spring->previous - current) - v * spring->friction;

	spring->current =
		current + (current - spring->previous) + force * step * step;
	spring->previous = current;

#if 0
	if (spring->current >= 1.0) {
#ifdef TWEENER_BOUNCE
		spring->current = 2.0 - spring->current;
		spring->previous = 2.0 - spring->previous;
#else
		spring->current = 1.0;
		spring->previous = 1.0;
#endif
	}

	if (spring->current <= 0.0) {
		spring->current = 0.0;
		spring->previous = 0.0;
	}
#endif
}

WL_EXPORT int
wlsc_spring_done(struct wlsc_spring *spring)
{
	return fabs(spring->previous - spring->target) < 0.0002 &&
		fabs(spring->current - spring->target) < 0.0002;
}
