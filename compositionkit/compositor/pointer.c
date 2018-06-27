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

void
wlsc_create_pointer_images(struct wlsc_compositor *ec)
{
	int i, count;

	count = ARRAY_LENGTH(pointer_images);
	ec->pointer_sprites = malloc(count * sizeof *ec->pointer_sprites);
	for (i = 0; i < count; i++) {
		fprintf (stderr, "attaching pointer sprite from %s\n", pointer_images[i].filename);

		ec->pointer_sprites[i] =
			wlsc_create_sprite_from_png(ec,
					       pointer_images[i].filename,
					       SPRITE_USE_CURSOR);
	}
}
