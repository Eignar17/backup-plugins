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

#include <stdlib.h>

#include "compositor.h"
#include "screenshooter-server-protocol.h"

struct screenshooter {
	struct wl_object base;
	struct wlsc_compositor *ec;
};

static void
screenshooter_shoot(struct wl_client *client,
		    struct screenshooter *shooter,
		    struct wl_output *output_base, struct wl_buffer *buffer)
{
	struct wlsc_output *output = (struct wlsc_output *) output_base;

	if (!wl_buffer_is_shm(buffer))
		return;

	if (buffer->width < output->width || buffer->height < output->height)
		return;

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, output->width, output->height,
		     GL_RGBA, GL_UNSIGNED_BYTE,
		     wl_shm_buffer_get_data(buffer));
}

struct screenshooter_interface screenshooter_implementation = {
	screenshooter_shoot
};

void
screenshooter_create(struct wlsc_compositor *ec)
{
	struct screenshooter *shooter;

	shooter = malloc(sizeof *shooter);
	if (shooter == NULL)
		return;

	shooter->base.interface = &screenshooter_interface;
	shooter->base.implementation =
		(void(**)(void)) &screenshooter_implementation;
	shooter->ec = ec;

	wl_display_add_object(ec->wl_display, &shooter->base);
	wl_display_add_global(ec->wl_display, &shooter->base, NULL);
};
