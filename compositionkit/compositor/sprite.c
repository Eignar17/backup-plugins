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

struct wlsc_sprite *
wlsc_create_sprite_from_pixel_data (struct wlsc_compositor *ec, int32_t width, int32_t height,
				    uint32_t stride, uint32_t usage, unsigned char *pixels)
{
	struct wlsc_sprite *sprite;

	if (pixels == NULL)
		return NULL;

	sprite = malloc(sizeof *sprite);
	if (sprite == NULL) {
		free(pixels);
		return NULL;
	}

	sprite->visual = &ec->compositor.premultiplied_argb_visual;
	sprite->width = width;
	sprite->height = height;
	sprite->image = EGL_NO_IMAGE_KHR;

	if (usage & SPRITE_USE_CURSOR && ec->create_cursor_image != NULL)
		sprite->image = ec->create_cursor_image(ec, width, height);

	glGenTextures(1, &sprite->texture);
	glBindTexture(GL_TEXTURE_2D, sprite->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (sprite->image != EGL_NO_IMAGE_KHR) {
		ec->image_target_texture_2d(GL_TEXTURE_2D, sprite->image);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
				GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
	} else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0,
			     GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
	}

	return sprite;
}


struct wlsc_sprite *
wlsc_create_sprite_from_png(struct wlsc_compositor *ec,
		       const char *filename, uint32_t usage)
{
	uint32_t *pixels;
	struct wlsc_sprite *sprite;
	int32_t width, height;
	uint32_t stride;

	pixels = wlsc_load_image(filename, &width, &height, &stride);
	if (pixels == NULL)
		return NULL;

	sprite = wlsc_create_sprite_from_pixel_data (ec, width, height, stride, usage, (unsigned char *) pixels);

	free (pixels);

	return sprite;
}
