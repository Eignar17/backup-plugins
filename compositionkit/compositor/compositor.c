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
#include "shaders.h"

char *option_background = "background.jpg";

/* The plan here is to generate a random anonymous socket name and
 * advertise that through a service on the session dbus.
 */
static const char *option_socket_name = NULL;
static int option_idle_time = 300;

WL_EXPORT void
wlsc_compositor_damage_all(struct wlsc_compositor *compositor)
{
	struct wlsc_output *output;

	wl_list_for_each(output, &compositor->output_list, link)
		wlsc_output_damage(output);
}

static int
repaint(void *data)
{
	struct wlsc_compositor *ec = data;
	struct wlsc_output *output;
	int repainted_all_outputs = 1;

	wl_list_for_each(output, &ec->output_list, link) {
		if (!output->repaint_needed)
			continue;

		if (!output->finished) {
			repainted_all_outputs = 0;
			continue;
		}

		wlsc_output_repaint(output);
		output->finished = 0;
		output->repaint_needed = 0;
		output->present(output);
	}

	if (repainted_all_outputs)
		ec->repaint_on_timeout = 0;
	else
		wl_event_source_timer_update(ec->timer_source, 1);

	return 1;
}

WL_EXPORT void
wlsc_compositor_schedule_repaint(struct wlsc_compositor *compositor)
{
	struct wlsc_output *output;

	if (compositor->state == WLSC_COMPOSITOR_SLEEPING)
		return;

	wl_list_for_each(output, &compositor->output_list, link)
		output->repaint_needed = 1;

	if (compositor->repaint_on_timeout)
		return;

	wl_event_source_timer_update(compositor->timer_source, 1);
	compositor->repaint_on_timeout = 1;
}

static void
surface_destroy(struct wl_client *client,
		struct wl_surface *surface)
{
	wl_resource_destroy(&surface->resource, client,
			    wlsc_compositor_get_time());
}

const static struct wl_surface_interface surface_interface = {
	wlsc_surface_destroy,
	wlsc_surface_attach,
	wlsc_surface_map_toplevel,
	wlsc_surface_map_transient,
	wlsc_surface_map_fullscreen,
	wlsc_surface_damage
};

static void
compositor_create_surface(struct wl_client *client,
			  struct wl_compositor *compositor, uint32_t id)
{
	struct wlsc_compositor *ec = (struct wlsc_compositor *) compositor;
	struct wlsc_surface *surface;

	surface = wlsc_surface_create(ec, 0, 0, 0, 0);
	if (surface == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	surface->surface.resource.destroy = wlsc_surface_destroy;

	surface->surface.resource.object.id = id;
	surface->surface.resource.object.interface = &wl_surface_interface;
	surface->surface.resource.object.implementation =
		(void (**)(void)) &surface_interface;
	surface->surface.client = client;

	wl_client_add_resource(client, &surface->surface.resource);
}

const static struct wl_compositor_interface compositor_interface = {
	compositor_create_surface,
};

static void
shm_buffer_created(struct wl_buffer *buffer)
{
	struct wl_list *surfaces_attached_to;

	surfaces_attached_to = malloc(sizeof *surfaces_attached_to);
	if (!surfaces_attached_to) {
		buffer->user_data = NULL;
		return;
	}

	wl_list_init(surfaces_attached_to);

	buffer->user_data = surfaces_attached_to;
}

static void
shm_buffer_damaged(struct wl_buffer *buffer,
		   int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct wl_list *surfaces_attached_to = buffer->user_data;
	struct wlsc_surface *es;
	GLsizei tex_width = wl_shm_buffer_get_stride(buffer) / 4;

	wl_list_for_each(es, surfaces_attached_to, buffer_link) {
		glBindTexture(GL_TEXTURE_2D, es->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
			     tex_width, buffer->height, 0,
			     GL_BGRA_EXT, GL_UNSIGNED_BYTE,
			     wl_shm_buffer_get_data(buffer));
		/* Hmm, should use glTexSubImage2D() here but GLES2 doesn't
		 * support any unpack attributes except GL_UNPACK_ALIGNMENT. */
	}
}

static void
shm_buffer_destroyed(struct wl_buffer *buffer)
{
	struct wl_list *surfaces_attached_to = buffer->user_data;
	struct wlsc_surface *es, *next;

	wl_list_for_each_safe(es, next, surfaces_attached_to, buffer_link) {
		wl_list_remove(&es->buffer_link);
		wl_list_init(&es->buffer_link);
	}

	free(surfaces_attached_to);
}

const static struct wl_shm_callbacks shm_callbacks = {
	shm_buffer_created,
	shm_buffer_damaged,
	shm_buffer_destroyed
};

static void
terminate_binding(struct wl_input_device *device, uint32_t time,
		  uint32_t key, uint32_t button, uint32_t state, void *data)
{
	struct wlsc_compositor *compositor = data;

	if (state)
		wl_display_terminate(compositor->wl_display);
}

WL_EXPORT int
wlsc_compositor_init(struct wlsc_compositor *ec, struct wl_display *display)
{
	struct wl_event_loop *loop;
	const char *extensions;

	ec->wl_display = display;

	wl_compositor_init(&ec->compositor, &compositor_interface, display);

	ec->shm = wl_shm_init(display, &shm_callbacks);

	ec->image_target_texture_2d =
		(void *) eglGetProcAddress("glEGLImageTargetTexture2DOES");
	ec->image_target_renderbuffer_storage = (void *)
		eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES");
	ec->create_image = (void *) eglGetProcAddress("eglCreateImageKHR");
	ec->destroy_image = (void *) eglGetProcAddress("eglDestroyImageKHR");
	ec->bind_display =
		(void *) eglGetProcAddress("eglBindWaylandDisplayWL");
	ec->unbind_display =
		(void *) eglGetProcAddress("eglUnbindWaylandDisplayWL");

	extensions = (const char *) glGetString(GL_EXTENSIONS);
	if (!strstr(extensions, "GL_EXT_texture_format_BGRA8888")) {
		fprintf(stderr,
			"GL_EXT_texture_format_BGRA8888 not available\n");
		return -1;
	}

	extensions =
		(const char *) eglQueryString(ec->display, EGL_EXTENSIONS);
	if (strstr(extensions, "EGL_WL_bind_wayland_display"))
		ec->has_bind_display = 1;
	if (ec->has_bind_display)
		ec->bind_display(ec->display, ec->wl_display);

	wl_list_init(&ec->surface_list);
	wl_list_init(&ec->input_device_list);
	wl_list_init(&ec->output_list);
	wl_list_init(&ec->binding_list);
	wl_list_init(&ec->animation_list);

	wlsc_compositor_add_binding(ec, KEY_BACKSPACE, 0,
				    MODIFIER_CTRL | MODIFIER_ALT,
				    terminate_binding, ec);

	wlsc_create_pointer_images(ec);

	glActiveTexture(GL_TEXTURE0);

	if (wlsc_shader_init(&ec->texture_shader,
			     vertex_shader, texture_fragment_shader) < 0)
		return -1;
	if (init_solid_shader(&ec->solid_shader,
			      ec->texture_shader.vertex_shader,
			      solid_fragment_shader) < 0)
		return -1;

	loop = wl_display_get_event_loop(ec->wl_display);

	ec->timer_source = wl_event_loop_add_timer(loop, repaint, ec);
	pixman_region32_init(&ec->damage_region);
	wlsc_compositor_schedule_repaint(ec);

	return 0;
}

static int on_term_signal(int signal_number, void *data)
{
	struct wlsc_compositor *ec = data;

	wl_display_terminate(ec->wl_display);

	return 1;
}

static void *
load_module(const char *name, const char *entrypoint, void **handle)
{
	char path[PATH_MAX];
	void *module, *init;

	if (name[0] != '/')
		snprintf(path, sizeof path, MODULEDIR "/%s", name);
	else
		snprintf(path, sizeof path, "%s", name);

	module = dlopen(path, RTLD_LAZY);
	if (!module) {
		fprintf(stderr,
			"failed to load module: %s\n", dlerror());
		return NULL;
	}

	init = dlsym(module, entrypoint);
	if (!init) {
		fprintf(stderr,
			"failed to lookup init function: %s\n", dlerror());
		return NULL;
	}

	return init;
}

int main(int argc, char *argv[])
{
	struct wl_display *display;
	struct wlsc_compositor *ec;
	struct wl_event_loop *loop;
	int o;
	void *backend_module;
	struct wlsc_compositor
		*(*backend_init)(struct wl_display *display, char *options);
	char *backend = NULL;
	char *backend_options = "";
	char *p;

	static const char opts[] = "B:b:o:S:i:s:";
	static const struct option longopts[ ] = {
		{ "backend", 1, NULL, 'B' },
		{ "backend-options", 1, NULL, 'o' },
		{ "background", 1, NULL, 'b' },
		{ "socket", 1, NULL, 'S' },
		{ "idle-time", 1, NULL, 'i' },
		{ NULL, }
	};

	while (o = getopt_long(argc, argv, opts, longopts, &o), o > 0) {
		switch (o) {
		case 'b':
			option_background = optarg;
			break;
		case 'B':
			backend = optarg;
			break;
		case 'o':
			backend_options = optarg;
			break;
		case 'S':
			option_socket_name = optarg;
			break;
		case 'i':
			option_idle_time = strtol(optarg, &p, 0);
			if (*p != '\0') {
				fprintf(stderr,
					"invalid idle time option: %s\n",
					optarg);
				exit(EXIT_FAILURE);
			}
			break;
			break;
		}
	}

	display = wl_display_create();

	ec = NULL;

	if (!backend) {
		if (getenv("WAYLAND_DISPLAY"))
			backend = "libwayland-backend.so";
		else if (getenv("DISPLAY"))
			backend = "libx11-backend.so";
		else if (getenv("OPENWFD"))
			backend = "libopenwfd-backend.so";
		else
			backend = "libdrm-backend.so";
	}

	backend_init = load_module(backend, "backend_init", &backend_module);
	if (!backend_init)
		exit(EXIT_FAILURE);

	ec = backend_init(display, backend_options);
	if (ec == NULL) {
		fprintf(stderr, "failed to create compositor\n");
		exit(EXIT_FAILURE);
	}

	if (wl_display_add_socket(display, option_socket_name)) {
		fprintf(stderr, "failed to add socket: %m\n");
		exit(EXIT_FAILURE);
	}

	loop = wl_display_get_event_loop(ec->wl_display);
	wl_event_loop_add_signal(loop, SIGTERM, on_term_signal, ec);
	wl_event_loop_add_signal(loop, SIGINT, on_term_signal, ec);

	wl_display_run(display);

	if (ec->has_bind_display)
		ec->unbind_display(ec->display, display);
	wl_display_destroy(display);

	ec->destroy(ec);

	return 0;
}
