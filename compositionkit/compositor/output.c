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

#define _GNU_SOURCE

#define DEG2RAD(x) (M_PI/180.0) * x

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <linux/input.h>
#include <dlfcn.h>
#include <getopt.h>
#include <signal.h>

#include "wayland-server.h"
#include "compositor.h"


WL_EXPORT void
wlsc_output_finish_frame(struct wlsc_output *output, int msecs)
{
	struct wlsc_compositor *compositor = output->compositor;
	struct wlsc_surface *es;
	struct wlsc_animation *animation, *next;

	wl_list_for_each(es, &compositor->surface_list, link) {
		if (es->output == output) {
			wl_display_post_frame(compositor->wl_display,
					      &es->surface, msecs);
		}
	}

	output->finished = 1;

	wl_event_source_timer_update(compositor->timer_source, 5);
	compositor->repaint_on_timeout = 1;

	wl_list_for_each_safe(animation, next,
			      &compositor->animation_list, link)
		animation->frame(animation, output, msecs);
}

WL_EXPORT void
wlsc_output_damage(struct wlsc_output *output)
{
	struct wlsc_compositor *compositor = output->compositor;

	pixman_region32_union_rect(&compositor->damage_region,
				   &compositor->damage_region,
				   output->x, output->y,
				   output->width, output->height);
	wlsc_compositor_schedule_repaint(compositor);
}
 
WL_EXPORT void
wlsc_output_repaint(struct wlsc_output *output)
{
	struct wlsc_compositor *ec = output->compositor;
	struct wlsc_surface *es;
	struct wlsc_input_device *eid, *hw_cursor;
	pixman_region32_t new_damage, total_damage;

	output->prepare_render(output);

	glViewport(0, 0, output->width, output->height);

	glUseProgram(ec->texture_shader.program);
	glUniformMatrix4fv(ec->texture_shader.proj_uniform,
			   1, GL_FALSE, output->matrix.d);
	glUniform1i(ec->texture_shader.tex_uniform, 0);

	pixman_region32_init(&new_damage);
	pixman_region32_init(&total_damage);
	pixman_region32_intersect_rect(&new_damage,
				       &ec->damage_region,
				       output->x, output->y,
				       output->width, output->height);
	pixman_region32_subtract(&ec->damage_region,
				 &ec->damage_region, &new_damage);
	pixman_region32_union(&total_damage, &new_damage,
			      &output->previous_damage_region);
	pixman_region32_copy(&output->previous_damage_region, &new_damage);

	hw_cursor = NULL;
	if (ec->focus) {
		hw_cursor = (struct wlsc_input_device *) ec->input_device;
		if (output->set_hardware_cursor(output, hw_cursor) < 0)
			hw_cursor = NULL;
	} else {
		output->set_hardware_cursor(output, NULL);
	}

	es = container_of(ec->surface_list.next, struct wlsc_surface, link);

	if (es->visual == &ec->compositor.rgb_visual && hw_cursor) {
		if (output->prepare_scanout_surface(output, es) == 0) {
			/* We're drawing nothing now,
			 * draw the damaged regions later. */
			pixman_region32_union(&ec->damage_region,
					      &ec->damage_region,
					      &total_damage);
			return;
		}
	}

	if (es->fullscreen_output == output) {
		if (es->width < output->width ||
		    es->height < output->height)
			glClear(GL_COLOR_BUFFER_BIT);
		wlsc_surface_draw(es, output, &total_damage);
	} else {
		if (output->background)
			wlsc_surface_draw(output->background,
					  output, &total_damage);

		glUseProgram(ec->texture_shader.program);
		wl_list_for_each_reverse(es, &ec->surface_list, link) {
			if (ec->overlay == es)
				continue;

			wlsc_surface_draw(es, output, &total_damage);
		}
	}

	if (ec->overlay)
		wlsc_surface_draw(ec->overlay, output, &total_damage);

	if (ec->focus)
		wl_list_for_each(eid, &ec->input_device_list, link) {
			if (&eid->input_device != ec->input_device ||
			    eid != hw_cursor)
				wlsc_surface_draw(eid->sprite, output,
						  &total_damage);
		}
}

static void
wlsc_output_post_geometry(struct wl_client *client,
			  struct wl_object *global, uint32_t version)
{
	struct wlsc_output *output =
		container_of(global, struct wlsc_output, object);

	wl_client_post_event(client, global,
			     WL_OUTPUT_GEOMETRY,
			     output->x, output->y,
			     output->width, output->height);
}


WL_EXPORT void
wlsc_output_destroy(struct wlsc_output *output)
{
	wlsc_surface_destroy (&output->background->surface.resource, NULL);
}

WL_EXPORT void
wlsc_output_move(struct wlsc_output *output, int x, int y)
{
	struct wlsc_compositor *c = output->compositor;
	int flip;

	output->x = x;
	output->y = y;

	if (output->background) {
		output->background->x = x;
		output->background->y = y;
	}

	pixman_region32_init(&output->previous_damage_region);

	wlsc_matrix_init(&output->matrix);
	wlsc_matrix_translate(&output->matrix,
			      -output->x - output->width / 2.0,
			      -output->y - output->height / 2.0, 0);

	flip = (output->flags & WL_OUTPUT_FLIPPED) ? -1 : 1;
	wlsc_matrix_scale(&output->matrix,
			  2.0 / output->width,
			  flip * 2.0 / output->height, 1);

	pixman_region32_union_rect(&c->damage_region,
				   &c->damage_region,
				   x, y, output->width, output->height);
}

static struct wlsc_surface *
background_create(struct wlsc_output *output, const char *filename)
{
	struct wlsc_surface *background;
	struct wlsc_sprite *sprite;

	background = wlsc_surface_create(output->compositor,
					 output->x, output->y,
					 output->width, output->height);
	if (background == NULL)
		return NULL;

	sprite = wlsc_create_sprite_from_png(output->compositor, filename, 0);
	if (sprite == NULL) {
		unsigned char pixels[4] = { 0x0, 0x0, 0x0, 0xff };
		sprite = wlsc_create_sprite_from_pixel_data (output->compositor, 1, 1, 0, 0, pixels);

		if (sprite == NULL)
		    return NULL;
	}

	wlsc_sprite_attach(sprite, &background->surface);

	return background;
}

WL_EXPORT void
wlsc_output_init(struct wlsc_output *output, struct wlsc_compositor *c,
		 int x, int y, int width, int height, uint32_t flags)
{
	output->compositor = c;
	output->x = x;
	output->y = y;
	output->width = width;
	output->height = height;

	output->background =
		background_create(output, option_background);

	output->flags = flags;
	output->finished = 1;
	wlsc_output_move(output, x, y);

	output->object.interface = &wl_output_interface;
	wl_display_add_object(c->wl_display, &output->object);
	wl_display_add_global(c->wl_display, &output->object,
			      wlsc_output_post_geometry);
}
