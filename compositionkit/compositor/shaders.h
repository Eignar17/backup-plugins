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

#ifndef _COMPOSITIONKIT_SHADERS_H
#define _COMPOSITIONKIT_SHADERS_H

static const char vertex_shader[] =
	"uniform mat4 proj;\n"
	"attribute vec2 position;\n"
	"attribute vec2 texcoord;\n"
	"varying vec2 v_texcoord;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = proj * vec4(position, 0.0, 1.0);\n"
	"   v_texcoord = texcoord;\n"
	"}\n";

static const char texture_fragment_shader[] =
	"precision mediump float;\n"
	"varying vec2 v_texcoord;\n"
	"uniform sampler2D tex;\n"
	"uniform float amount;\n"
	"void main()\n"
	"{\n"
	"   vec4 color = texture2D(tex, v_texcoord);\n"
	"   float shift = sin (amount);\n"
	"   gl_FragColor = color\n;"
	"}\n";

static const char solid_fragment_shader[] =
	"precision mediump float;\n"
	"uniform vec4 color;\n"
	"void main()\n"
	"{\n"
	"   gl_FragColor = color\n;"
	"}\n";

int
wlsc_shader_init(struct wlsc_shader *shader,
		 const char *vertex_source, const char *fragment_source);

int
init_solid_shader(struct wlsc_shader *shader,
		  GLuint vertex_shader, const char *fragment_source);

#endif
