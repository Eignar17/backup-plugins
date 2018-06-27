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

WL_EXPORT struct wlsc_surface *
wlsc_surface_create(struct wlsc_compositor *compositor,
		    int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct wlsc_surface *surface;

	surface = malloc(sizeof *surface);
	if (surface == NULL)
		return NULL;

	wl_list_init(&surface->link);
	wl_list_init(&surface->buffer_link);
	surface->map_type = WLSC_SURFACE_MAP_UNMAPPED;

	glGenTextures(1, &surface->texture);
	glBindTexture(GL_TEXTURE_2D, surface->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	surface->compositor = compositor;
	surface->visual = NULL;
	surface->image = EGL_NO_IMAGE_KHR;
	surface->saved_texture = 0;
	surface->x = x;
	surface->y = y;
	surface->width = width;
	surface->height = height;

	surface->transform = NULL;

	return surface;
}

WL_EXPORT void
wlsc_surface_damage_rectangle(struct wlsc_surface *surface,
			      int32_t x, int32_t y,
			      int32_t width, int32_t height)
{
	struct wlsc_compositor *compositor = surface->compositor;

	pixman_region32_union_rect(&compositor->damage_region,
				   &compositor->damage_region,
				   surface->x + x, surface->y + y,
				   width, height);
	wlsc_compositor_schedule_repaint(compositor);
}

WL_EXPORT void
wlsc_surface_damage_surface (struct wlsc_surface *surface)
{
	wlsc_surface_damage_rectangle(surface, 0, 0,
				      surface->width, surface->height);
}

WL_EXPORT uint32_t
wlsc_compositor_get_time(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

WL_EXPORT void
wlsc_surface_destroy (struct wl_resource *resource, struct wl_client *client)
{
	struct wlsc_surface *surface =
		container_of(resource, struct wlsc_surface, surface.resource);
	struct wlsc_compositor *compositor = surface->compositor;

	wlsc_surface_damage_surface (surface);

	wl_list_remove(&surface->link);
	if (surface->saved_texture == 0)
		glDeleteTextures(1, &surface->texture);
	else
		glDeleteTextures(1, &surface->saved_texture);


	if (surface->image != EGL_NO_IMAGE_KHR)
		compositor->destroy_image(compositor->display,
					  surface->image);

	wl_list_remove(&surface->buffer_link);

	free(surface);
}

WL_EXPORT void
wlsc_surface_attach(struct wl_client *client,
		    struct wl_surface *surface, struct wl_buffer *buffer,
		    int32_t x, int32_t y)
{
	struct wlsc_surface *es = (struct wlsc_surface *) surface;

	/* FIXME: This damages the entire old surface, but we should
	 * really just damage the part that's no longer covered by the
	 * surface.  Anything covered by the new surface will be
	 * damaged by the client. */
	wlsc_surface_damage_surface (es);

	switch (es->map_type) {
	case WLSC_SURFACE_MAP_FULLSCREEN:
		es->x = (es->fullscreen_output->width - es->width) / 2;
		es->y = (es->fullscreen_output->height - es->height) / 2;
		break;
	default:
		es->x += x;
		es->y += y;
		break;
	}
	es->width = buffer->width;
	es->height = buffer->height;
	if (x != 0 || y != 0)
		wlsc_surface_assign_output(es);

	wlsc_buffer_attach(buffer, surface);
}

WL_EXPORT void
wlsc_surface_map_toplevel(struct wl_client *client,
			  struct wl_surface *surface)
{
	struct wlsc_surface *es = (struct wlsc_surface *) surface;
	struct wlsc_compositor *ec = es->compositor;

	switch (es->map_type) {
	case WLSC_SURFACE_MAP_UNMAPPED:
		es->x = 10 + random() % 400;
		es->y = 10 + random() % 400;
		/* assign to first output */
		es->output = container_of(ec->output_list.next,
					  struct wlsc_output, link);
		wl_list_insert(&es->compositor->surface_list, &es->link);
		break;
	case WLSC_SURFACE_MAP_TOPLEVEL:
		return;
	case WLSC_SURFACE_MAP_FULLSCREEN:
		es->fullscreen_output = NULL;
		es->x = es->saved_x;
		es->y = es->saved_y;
		break;
	default:
		break;
	}

	wlsc_surface_damage_surface (es);
	es->map_type = WLSC_SURFACE_MAP_TOPLEVEL;
}

WL_EXPORT void
wlsc_surface_map_transient(struct wl_client *client,
			   struct wl_surface *surface, struct wl_surface *parent,
			   int x, int y, uint32_t flags)
{
	struct wlsc_surface *es = (struct wlsc_surface *) surface;
	struct wlsc_surface *pes = (struct wlsc_surface *) parent;

	switch (es->map_type) {
	case WLSC_SURFACE_MAP_UNMAPPED:
		wl_list_insert(&es->compositor->surface_list, &es->link);
		/* assign to parents output  */
		es->output = pes->output;
		break;
	case WLSC_SURFACE_MAP_FULLSCREEN:
		es->fullscreen_output = NULL;
		break;
	default:
		break;
	}

	es->x = pes->x + x;
	es->y = pes->y + y;

	wlsc_surface_damage_surface (es);
	es->map_type = WLSC_SURFACE_MAP_TRANSIENT;
}

WL_EXPORT void
wlsc_surface_map_fullscreen (struct wl_client *client, struct wl_surface *surface)
{
	struct wlsc_surface *es = (struct wlsc_surface *) surface;
	struct wlsc_output *output;

	/* FIXME: Fullscreen on first output */
	/* FIXME: Handle output going away */
	output = container_of(es->compositor->output_list.next,
			      struct wlsc_output, link);

	switch (es->map_type) {
	case WLSC_SURFACE_MAP_UNMAPPED:
		es->x = 10 + random() % 400;
		es->y = 10 + random() % 400;
		/* assign to first output */
		es->output = output;
		wl_list_insert(&es->compositor->surface_list, &es->link);
		break;
	case WLSC_SURFACE_MAP_FULLSCREEN:
		return;
	default:
		break;
	}

	es->saved_x = es->x;
	es->saved_y = es->y;
	es->x = (output->width - es->width) / 2;
	es->y = (output->height - es->height) / 2;
	es->fullscreen_output = output;
	wlsc_surface_damage_surface (es);
	es->map_type = WLSC_SURFACE_MAP_FULLSCREEN;
}

WL_EXPORT void
wlsc_surface_damage(struct wl_client *client,
		    struct wl_surface *surface,
		    int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct wlsc_surface *es = (struct wlsc_surface *) surface;

	wlsc_surface_damage_rectangle(es, x, y, width, height);
}

WL_EXPORT void
wlsc_buffer_attach(struct wl_buffer *buffer, struct wl_surface *surface)
{
	struct wlsc_surface *es = (struct wlsc_surface *) surface;
	struct wlsc_compositor *ec = es->compositor;
	struct wl_list *surfaces_attached_to;

	if (es->saved_texture != 0)
		es->texture = es->saved_texture;

	glBindTexture(GL_TEXTURE_2D, es->texture);

	if (wl_buffer_is_shm(buffer)) {
		/* Unbind any EGLImage texture that may be bound, so we don't
		 * overwrite it.*/
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
			     0, 0, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		es->pitch = wl_shm_buffer_get_stride(buffer) / 4;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
			     es->pitch, buffer->height, 0,
			     GL_BGRA_EXT, GL_UNSIGNED_BYTE,
			     wl_shm_buffer_get_data(buffer));
		es->visual = buffer->visual;

		surfaces_attached_to = buffer->user_data;

		wl_list_remove(&es->buffer_link);
		wl_list_insert(surfaces_attached_to, &es->buffer_link);
	} else {
		es->image = ec->create_image(ec->display, NULL,
					     EGL_WAYLAND_BUFFER_WL,
					     buffer, NULL);
		
		ec->image_target_texture_2d(GL_TEXTURE_2D, es->image);
		es->visual = buffer->visual;
		es->pitch = es->width;
	}
}

WL_EXPORT void
wlsc_sprite_attach(struct wlsc_sprite *sprite, struct wl_surface *surface)
{
	struct wlsc_surface *es = (struct wlsc_surface *) surface;
	struct wlsc_compositor *ec = es->compositor;

	es->pitch = es->width;
	es->image = sprite->image;
	if (sprite->image != EGL_NO_IMAGE_KHR) {
		glBindTexture(GL_TEXTURE_2D, es->texture);
		ec->image_target_texture_2d(GL_TEXTURE_2D, es->image);
	} else {
		if (es->saved_texture == 0)
			es->saved_texture = es->texture;
		es->texture = sprite->texture;
	}

	es->visual = sprite->visual;
}

WL_EXPORT void
wlsc_surface_transform(struct wlsc_surface *surface,
		       int32_t x, int32_t y, int32_t *sx, int32_t *sy)
{
	*sx = x - surface->x;
	*sy = y - surface->y;
}

WL_EXPORT void
wlsc_surface_activate(struct wlsc_surface *surface,
		      struct wlsc_input_device *device, uint32_t time)
{
	wlsc_surface_raise(surface);

	wl_input_device_set_keyboard_focus(&device->input_device,
					   &surface->surface,
					   time);
}

static void
transform_vertex(struct wlsc_surface *surface,
		 GLfloat x, GLfloat y, GLfloat u, GLfloat v, GLfloat *r)
{
	struct wlsc_vector t;

	t.f[0] = x;
	t.f[1] = y;
	t.f[2] = 0.0;
	t.f[3] = 1.0;

	wlsc_matrix_transform(&surface->transform->matrix, &t);

	r[ 0] = t.f[0];
	r[ 1] = t.f[1];
	r[ 2] = u;
	r[ 3] = v;
}

static int
texture_region(struct wlsc_surface *es, pixman_region32_t *region)
{
	struct wlsc_compositor *ec = es->compositor;
	GLfloat *v, inv_width, inv_height;
	pixman_box32_t *rectangles;
	unsigned int *p;
	int i, n;

	rectangles = pixman_region32_rectangles(region, &n);
	v = wl_array_add(&ec->vertices, n * 16 * sizeof *v);
	p = wl_array_add(&ec->indices, n * 6 * sizeof *p);
	inv_width = 1.0 / es->pitch;
	inv_height = 1.0 / es->height;

	for (i = 0; i < n; i++, v += 16, p += 6) {
		v[ 0] = rectangles[i].x1;
		v[ 1] = rectangles[i].y1;
		v[ 2] = (GLfloat) (rectangles[i].x1 - es->x) * inv_width;
		v[ 3] = (GLfloat) (rectangles[i].y1 - es->y) * inv_height;

		v[ 4] = rectangles[i].x1;
		v[ 5] = rectangles[i].y2;
		v[ 6] = v[ 2];
		v[ 7] = (GLfloat) (rectangles[i].y2 - es->y) * inv_height;

		v[ 8] = rectangles[i].x2;
		v[ 9] = rectangles[i].y1;
		v[10] = (GLfloat) (rectangles[i].x2 - es->x) * inv_width;
		v[11] = v[ 3];

		v[12] = rectangles[i].x2;
		v[13] = rectangles[i].y2;
		v[14] = v[10];
		v[15] = v[ 7];

		p[0] = i * 4 + 0;
		p[1] = i * 4 + 1;
		p[2] = i * 4 + 2;
		p[3] = i * 4 + 2;
		p[4] = i * 4 + 1;
		p[5] = i * 4 + 3;
	}

	return n;
}

static int
texture_transformed_surface(struct wlsc_surface *es)
{
	struct wlsc_compositor *ec = es->compositor;
	GLfloat *v;
	unsigned int *p;

	v = wl_array_add(&ec->vertices, 16 * sizeof *v);
	p = wl_array_add(&ec->indices, 6 * sizeof *p);

	transform_vertex(es, es->x, es->y, 0.0, 0.0, &v[0]);
	transform_vertex(es, es->x, es->y + es->height, 0.0, 1.0, &v[4]);
	transform_vertex(es, es->x + es->width, es->y, 1.0, 0.0, &v[8]);
	transform_vertex(es, es->x + es->width, es->y + es->height,
			 1.0, 1.0, &v[12]);

	p[0] = 0;
	p[1] = 1;
	p[2] = 2;
	p[3] = 2;
	p[4] = 1;
	p[5] = 3;

	return 1;
}

WL_EXPORT void
wlsc_surface_draw(struct wlsc_surface *es,
		  struct wlsc_output *output, pixman_region32_t *clip)
{
	struct wlsc_compositor *ec = es->compositor;
	GLfloat *v;
	pixman_region32_t repaint;
	int n;

	pixman_region32_init_rect(&repaint,
				  es->x, es->y, es->width, es->height);
	pixman_region32_intersect(&repaint, &repaint, clip);
	if (!pixman_region32_not_empty(&repaint))
		return;

	if (es->visual == &ec->compositor.argb_visual) {
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	} else if (es->visual == &ec->compositor.premultiplied_argb_visual) {
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}

	if (es->transform == NULL)
		n = texture_region(es, &repaint);
	else
		n = texture_transformed_surface(es);

	glBindTexture(GL_TEXTURE_2D, es->texture);
	v = ec->vertices.data;
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof *v, &v[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof *v, &v[2]);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDrawElements(GL_TRIANGLES, n * 6, GL_UNSIGNED_INT, ec->indices.data);

	ec->vertices.size = 0;
	ec->indices.size = 0;
	pixman_region32_fini(&repaint);
}

WL_EXPORT void
wlsc_surface_raise(struct wlsc_surface *surface)
{
	struct wlsc_compositor *compositor = surface->compositor;

	wl_list_remove(&surface->link);
	wl_list_insert(&compositor->surface_list, &surface->link);
}

WL_EXPORT void
wlsc_surface_assign_output(struct wlsc_surface *es)
{
	struct wlsc_compositor *ec = es->compositor;
	struct wlsc_output *output;

	struct wlsc_output *tmp = es->output;
	es->output = NULL;

	wl_list_for_each(output, &ec->output_list, link) {
		if (output->x < es->x && es->x < output->x + output->width &&
		    output->y < es->y && es->y < output->y + output->height) {
			if (output != tmp)
				printf("assiging surface %p to output %p\n",
				       es, output);
			es->output = output;
		}
	}
	
	if (es->output == NULL) {
		printf("no output found\n");
		es->output = container_of(ec->output_list.next,
					  struct wlsc_output, link);
	}
}
