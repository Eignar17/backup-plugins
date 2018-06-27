/*
 * Copyright © 2010 Canonical Ltd
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

#include "compositionkit-service.h"

#ifndef _COMPOSITIONKIT_SERVICE_H_
#define _COMPOSITIONKIT_SERVICE_H_

G_BEGIN_DECLS

typedef struct _CompositionKitServicePrivate   CompositionKitServicePrivate;
typedef struct _CompositionKitSurfacePrivate   CompositionKitSurfacePrivate;
typedef struct _CompositionKitShaderPrivate    CompositionKitShaderPrivate;
typedef struct _CompositionKitUniformPrivate   CompositionKitUniformPrivate;
typedef struct _CompositionKitAttributePrivate CompositionKitAttributePrivate;

struct _CompositionKitServicePrivate
{
  CompositionKitObjectSkeleton *object;
  GList    		       *surfaces;
};

struct _CompositionKitSurfacePrivate
{
  CompositionKitServicePrivate *service;
  CompositionKitObjectSkeleton *object;
  gint      		       id;
  GList     		       *shaders;
};

typedef enum
{
  SHADER_TYPE_FRAGMENT = 0,
  SHADER_TYPE_VERTEX
} CKShaderType;

struct _CompositionKitUniformPrivate
{
  CompositionKitShaderPrivate  *shader;
  CompositionKitObjectSkeleton *object;
  gchar    *name;
  gchar    *type;
  GVariant *data;
};

struct _CompositionKitAttributePrivate
{
  CompositionKitShaderPrivate  *shader;
  CompositionKitObjectSkeleton *object;
  gchar    *name;
  gchar    *type;
  GVariant *data;
};

struct _CompositionKitShaderPrivate
{
  CompositionKitSurfacePrivate *surface;
  gchar                        *name;
  CKShaderType                 type;
  gchar                        *source;
  GList                        *uniforms;
  GList                        *attributes;
  CompositionKitObjectSkeleton *object;
};

G_END_DECLS

#endif

static GDBusObjectManagerServer *manager = NULL;

/* Methods */

static gint
find_surface_by_id (gconstpointer a,
		    gconstpointer b)
{
  CompositionKitSurfacePrivate *cksp = (CompositionKitSurfacePrivate *) a;
  gint		  *req_id = (gint *) b;

  return cksp->id == *req_id ? 0 : 1;
}

static gint
find_attribute_by_name (gconstpointer a,
		      gconstpointer b)
{
  CompositionKitAttributePrivate *ckap = (CompositionKitAttributePrivate *) a;
  gchar                          *req_name = (gchar *) b;

  return g_strcmp0 (ckap->name, req_name);
}

static gint
find_uniform_by_name (gconstpointer a,
		      gconstpointer b)
{
  CompositionKitUniformPrivate *ckup = (CompositionKitUniformPrivate *) a;
  gchar                        *req_name = (gchar *) b;

  return g_strcmp0 (ckup->name, req_name);
}

static gint
find_shader_by_name (gconstpointer a,
		     gconstpointer b)
{
  CompositionKitShaderPrivate  *cksp = (CompositionKitShaderPrivate *) a;
  gchar		        *req_name = (gchar *) b;

  return g_strcmp0 (cksp->name, req_name);
}

static GVariant *
list_shaders (CompositionKitSurfacePrivate *priv)
{
  GList 	    *list = priv->shaders;
  GVariantBuilder   *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  GVariant	    *return_value;

  if (!list)
    {
      g_variant_builder_unref (builder);
      return NULL;
    }

  while (list)
    {
      g_variant_builder_add (builder, "s", ((CompositionKitShaderPrivate *) list->data)->name);
      list = g_list_next (list);
    }

  return_value = g_variant_new ("as", builder);
  g_variant_builder_unref (builder);

  return return_value;
}

static GVariant *
list_surfaces (CompositionKitServicePrivate *priv)
{
  GList *list = priv->surfaces;
  GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("ai"));
  GVariant	    *return_value;

  if (!list)
    {
      g_variant_builder_unref (builder);
      return NULL;
    }

  while (list)
    {
      g_variant_builder_add (builder, "i", ((CompositionKitSurfacePrivate *) list->data)->id);
      list = g_list_next (list);
    }

  return_value = g_variant_new ("ai", builder);
  g_variant_builder_unref (builder);

  return return_value;
}

void
delete_uniform (CompositionKitUniformPrivate *priv)
{
  gchar          *s = g_strdup_printf ("/com/canonical/CompositionKit/Surface/%d/Shader/%s/Uniform/%s",
                                       priv->shader->surface->id, priv->shader->name, priv->name);

  if (priv->name)
    g_free (priv->name);

  if (priv->type)
    g_free (priv->type);

  if (priv->data)
    g_variant_unref (priv->data);

  g_dbus_object_manager_server_unexport (manager, s);

  g_free (s);
  g_free (priv);
}

void
delete_attribute (CompositionKitAttributePrivate *priv)
{
  gchar          *s = g_strdup_printf ("/com/canonical/CompositionKit/Surface/%d/Shader/%s/Attribute/%s",
                                       priv->shader->surface->id, priv->shader->name, priv->name);

  if (priv->name)
    g_free (priv->name);

  if (priv->type)
    g_free (priv->type);

  if (priv->data)
    g_variant_unref (priv->data);

  g_dbus_object_manager_server_unexport (manager, s);

  g_free (s);
  g_free (priv);
}

void
delete_shader (CompositionKitShaderPrivate *priv)
{
  GList		 *iter;
  gchar          *s = g_strdup_printf ("/com/canonical/CompositionKit/Surface/%d/Shader/%s", priv->surface->id, priv->name);

  if (priv->name)
    g_free (priv->name);

  if (priv->source)
    g_free (priv->source);

  iter = priv->uniforms;

  while (iter)
    {
      CompositionKitUniformPrivate *uniform = (CompositionKitUniformPrivate *) iter;
      delete_uniform (uniform);
      iter = g_list_remove_link (iter, iter);
    }

  iter = priv->attributes;

  while (iter)
    {
      CompositionKitAttributePrivate *attribute = (CompositionKitAttributePrivate *) iter;
      delete_attribute (attribute);
      iter = g_list_remove_link (iter, iter);
    }

  g_dbus_object_manager_server_unexport (manager, s);

  g_free (s);
  g_free (priv);
}  

void
delete_surface (CompositionKitSurfacePrivate *priv)
{
  GList		  *iter = priv->shaders;
  gchar           *s = g_strdup_printf ("/com/canonical/CompositionKit/Surface/%d", priv->id);


  while (iter)
    {
      CompositionKitShaderPrivate   *priv = (CompositionKitShaderPrivate *) iter;
      delete_shader (priv);
      iter = g_list_remove_link (iter, iter);
    }

  g_dbus_object_manager_server_unexport (manager, s);

  g_free (s);
  g_free (priv);
}

CompositionKitSurfacePrivate *
new_surface (gint id, CompositionKitObjectSkeleton *object, CompositionKitServicePrivate *service)
{
  CompositionKitSurfacePrivate *cksp = g_new0 (CompositionKitSurfacePrivate, 1);

  cksp->id = id;
  cksp->shaders = NULL;
  cksp->object = object;
  cksp->service = service;

  return cksp;
}

CompositionKitShaderPrivate *
new_shader (const gchar *name, gint shader_type, CompositionKitObjectSkeleton *object, CompositionKitSurfacePrivate *surface)
{
  CompositionKitShaderPrivate *cksp = g_new0 (CompositionKitShaderPrivate, 1);

  cksp->name = g_strdup (name);
  cksp->type = shader_type;
  cksp->source = NULL;
  cksp->uniforms = NULL;
  cksp->attributes = NULL;
  cksp->object = object;
  cksp->surface = surface;
}

CompositionKitUniformPrivate *
new_uniform (const gchar *name, const gchar *type, CompositionKitObjectSkeleton *object, CompositionKitShaderPrivate *shader)
{
  CompositionKitUniformPrivate *ckup = g_new0 (CompositionKitUniformPrivate, 1);

  ckup->name = g_strdup (name);
  ckup->type = g_strdup (type);
  ckup->object = object;
  ckup->shader = shader;
  ckup->data = NULL;

  return ckup;
}

CompositionKitAttributePrivate *
new_attribute (const gchar *name, const gchar *type, CompositionKitObjectSkeleton *object, CompositionKitShaderPrivate *shader)
{
  CompositionKitAttributePrivate *ckap = g_new0 (CompositionKitAttributePrivate, 1);

  ckap->name = g_strdup (name);
  ckap->type = g_strdup (type);
  ckap->object = object;
  ckap->shader = shader;
  ckap->data = NULL;

  return ckap;
}

static gboolean
on_handle_add_uniform (CompositionKitShader   *shader,
		       GDBusMethodInvocation  *invocation,
		       gchar		      *uniform_name,
		       gchar		      *uniform_type,
		       gpointer		      *user_data)
{
  CompositionKitShaderPrivate *priv = (CompositionKitShaderPrivate *) user_data;

  if (uniform_name && uniform_type)
    {
      /* create a new com.canonical.CompositionKit.Uniform object at
       * /com/canonical/CompositionKit/Surface/N/Shader/sname/Uniform/name where N is 000..
       * and sname is priv->name and name is uniform_name */

      gchar *s = g_strdup_printf ("/com/canonical/CompositionKit/Surface/%d/Shader/%s/Uniform/%s",
                                  priv->surface->id, priv->name, uniform_name);
      CompositionKitObjectSkeleton *object = composition_kit_object_skeleton_new (s);
      CompositionKitUniform         *uniform = composition_kit_uniform_skeleton_new ();
      CompositionKitUniformPrivate  *upriv  = new_uniform (uniform_name, uniform_type, object, priv);

      priv->uniforms = g_list_append (priv->uniforms, (gpointer) upriv);

      composition_kit_object_skeleton_set_uniform (object, uniform);
      g_object_unref (uniform);

      g_dbus_object_manager_server_export (manager, G_DBUS_OBJECT_SKELETON (object));
      g_object_unref (object);

      g_free (s);
      s = NULL;
    }

  composition_kit_shader_complete_add_uniform (shader, invocation);
}

static gboolean
on_handle_add_attribute (CompositionKitShader   *shader,
			 GDBusMethodInvocation  *invocation,
			 gchar		        *attribute_name,
			 gchar		        *attribute_type,
			 gpointer		*user_data)
{
  CompositionKitShaderPrivate *priv = (CompositionKitShaderPrivate *) user_data;

  if (priv->type != SHADER_TYPE_VERTEX)
    g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                           G_DBUS_ERROR_FAILED,
                                           "Attempted to add an attribute to a non-vertex shader. This is not allowed");
  else if (attribute_name && attribute_type)
    {
      /* create a new com.canonical.CompositionKit.Uniform object at
       * /com/canonical/CompositionKit/Surface/N/Shader/sname/Uniform/name where N is 000..
       * and sname is priv->name and name is uniform_name */

      gchar *s = g_strdup_printf ("/com/canonical/CompositionKit/Surface/%d/Shader/%s/Attribute/%s",
                                  priv->surface->id, priv->name, attribute_name);
      CompositionKitObjectSkeleton *object = composition_kit_object_skeleton_new (s);
      CompositionKitAttribute      *attribute = composition_kit_attribute_skeleton_new ();
      CompositionKitAttributePrivate  *apriv  = new_attribute (attribute_name, attribute_type, object, priv);

      priv->uniforms = g_list_append (priv->attributes, (gpointer) apriv);

      composition_kit_object_skeleton_set_attribute (object, attribute);
      g_object_unref (attribute);

      g_dbus_object_manager_server_export (manager, G_DBUS_OBJECT_SKELETON (object));
      g_object_unref (object);

      g_free (s);
      s = NULL;

      composition_kit_shader_complete_add_attribute (shader, invocation);
    }
}

static gboolean
on_handle_remove_attribute (CompositionKitShader   *shader,
			    GDBusMethodInvocation  *invocation,
			    gchar		   *attribute_name,
			    gpointer		   *user_data)
{
  CompositionKitShaderPrivate *priv = (CompositionKitShaderPrivate *) user_data;

  GList   *list;
  list = g_list_find_custom (priv->attributes, attribute_name, find_attribute_by_name);

  if (list)
    {
      CompositionKitAttributePrivate *apriv   = (CompositionKitAttributePrivate *) list->data;

      delete_attribute (apriv);
      priv->attributes = g_list_remove_link (priv->attributes, list);
    }

  composition_kit_shader_complete_remove_attribute (shader, invocation);
}

static gboolean
on_handle_remove_uniform (CompositionKitShader   *shader,
			  GDBusMethodInvocation  *invocation,
			  gchar		         *uniform_name,
			  gpointer		 *user_data)
{
  CompositionKitShaderPrivate *priv = (CompositionKitShaderPrivate *) user_data;

  GList   *list;
  list = g_list_find_custom (priv->uniforms, uniform_name, find_uniform_by_name);

  if (list)
    {
      CompositionKitUniformPrivate *upriv   = (CompositionKitUniformPrivate *) list->data;

      delete_uniform (upriv);
      priv->uniforms = g_list_remove_link (priv->uniforms, list);
    }

  composition_kit_shader_complete_remove_uniform (shader, invocation);
}

static gboolean
on_handle_set_shader_source (CompositionKitShader  *shader,
			     GDBusMethodInvocation *invocation,
			     gchar		   *source,
			     gpointer		   *user_data)
{
  CompositionKitShaderPrivate *priv = (CompositionKitShaderPrivate *) user_data;

  if (priv->source)
    {
      g_free (priv->source);
      priv->source = NULL;
    }

  if (source)
    {
      priv->source = g_strdup (source);
    }

  composition_kit_shader_complete_set_shader_source (shader, invocation);
}

static gboolean
on_handle_get_shader_source (CompositionKitShader  *shader,
			     GDBusMethodInvocation *invocation,
			     gpointer		   *user_data)
{
  CompositionKitShaderPrivate *priv = (CompositionKitShaderPrivate *) user_data;

  if (priv->source)
    {
      composition_kit_shader_complete_get_shader_source (shader, invocation, priv->source);
    }
  else
    {
      g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                             G_DBUS_ERROR_FAILED,
                                             "Shader %s does not have a source\n", priv->name);
    }
}

static gboolean
on_handle_add_shader (CompositionKitSurface  *surface,
		      GDBusMethodInvocation  *invocation,
		      gchar		     *shader_name,
		      gpointer		     *user_data)
{
  CompositionKitSurfacePrivate *priv = (CompositionKitSurfacePrivate *) user_data;

  if (shader_name)
    {
      /* create a new com.canonical.CompositionKit.Surface object at
       * /com/canonical/CompositionKit/Surface/N/Shader/name where N is 000..
       * and name is shader_name */

      gchar *s = g_strdup_printf ("/com/canonical/CompositionKit/Surface/%d/Shader/%s", priv->id, shader_name);
      CompositionKitObjectSkeleton *object = composition_kit_object_skeleton_new (s);
      CompositionKitShader         *shader = composition_kit_shader_skeleton_new ();
      CompositionKitShaderPrivate  *spriv  = new_shader (shader_name, SHADER_TYPE_FRAGMENT, object, priv);

      priv->shaders = g_list_append (priv->shaders, (gpointer) spriv);

      composition_kit_object_skeleton_set_shader (object, shader);
      g_object_unref (shader);

      g_free (s);
      s = NULL;

      g_signal_connect (G_OBJECT (shader), "handle-add-uniform",
                        G_CALLBACK (on_handle_add_uniform),
                        (gpointer) spriv);

      g_signal_connect (G_OBJECT (shader), "handle-remove-uniform",
                        G_CALLBACK (on_handle_remove_uniform),
                        (gpointer) spriv);

      g_signal_connect (G_OBJECT (shader), "handle-add-attribute",
                        G_CALLBACK (on_handle_add_attribute),
                        (gpointer) spriv);

      g_signal_connect (G_OBJECT (shader), "handle-remove-attribute",
                        G_CALLBACK (on_handle_remove_attribute),
                        (gpointer) spriv);

      g_signal_connect (G_OBJECT (shader), "handle-set-shader-source",
                        G_CALLBACK (on_handle_set_shader_source),
                        (gpointer) spriv);

      g_signal_connect (G_OBJECT (shader), "handle-get-shader-source",
                        G_CALLBACK (on_handle_get_shader_source),
                        (gpointer) spriv);


      g_dbus_object_manager_server_export (manager, G_DBUS_OBJECT_SKELETON (object));
      g_object_unref (object);
    }

  composition_kit_surface_complete_add_shader (surface, invocation);
}

static gboolean
on_handle_list_shaders (CompositionKitSurface *surface,
			GDBusMethodInvocation *invocation,
			gpointer	       *user_data)
{
  CompositionKitSurfacePrivate *priv = (CompositionKitSurfacePrivate *) user_data;
  GVariant                     *shaders = list_shaders (priv);
  gsize                        ret_size;
  const gchar                  **ret = g_variant_get_strv (shaders, &ret_size);


  if (ret)
    {
      composition_kit_surface_complete_list_shaders (surface, invocation, ret);
    }
  else
    {
      g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                             G_DBUS_ERROR_FAILED,
                                             "No shaders attached to surface %i\n", priv->id);
    }
}

static gboolean
on_handle_remove_shader (CompositionKitSurface *surface,
			  GDBusMethodInvocation *invocation,
			  gchar		        *shader_name,
			  gpointer		*user_data)
{
  CompositionKitSurfacePrivate *priv = (CompositionKitSurfacePrivate *) user_data;

  GList   *list;
  list = g_list_find_custom (priv->shaders, shader_name, find_shader_by_name);

  if (list)
    {
      CompositionKitShaderPrivate *spriv   = (CompositionKitShaderPrivate *) list->data;

      delete_shader (spriv);
      priv->shaders = g_list_remove_link (priv->shaders, list);
    }

  composition_kit_surface_complete_remove_shader (surface, invocation);
}

static gboolean
on_handle_add_surface (CompositionKitService *service,
		       GDBusMethodInvocation *invocation,
		       gint		     id,
		       gpointer		     *user_data)
{
  CompositionKitServicePrivate *priv = (CompositionKitServicePrivate *) user_data;

  if (id)
    {
      /* create a new com.canonical.CompositionKit.Surface object at
       * /com/canonical/CompositionKit/Surface/N where N is 000.. */

      gchar *s = g_strdup_printf ("/com/canonical/CompositionKit/Surface/%d", id);
      CompositionKitObjectSkeleton *object  = composition_kit_object_skeleton_new (s);
      CompositionKitSurface        *surface = composition_kit_surface_skeleton_new ();
      CompositionKitSurfacePrivate *spriv   = new_surface (id, object, priv);

      priv->surfaces = g_list_append (priv->surfaces, (gpointer) spriv);

      composition_kit_object_skeleton_set_surface (object, surface);
      g_object_unref (surface);

      g_free (s);
      s = NULL;

      g_signal_connect (G_OBJECT (surface), "handle-add-shader",
		        G_CALLBACK (on_handle_add_shader),
			(gpointer) spriv);

      g_signal_connect (G_OBJECT (surface), "handle-remove-shader",
		        G_CALLBACK (on_handle_remove_shader),
			(gpointer) spriv);

      g_signal_connect (G_OBJECT (surface), "handle-list-shaders",
		        G_CALLBACK (on_handle_list_shaders),
			(gpointer) spriv);

      g_dbus_object_manager_server_export (manager, G_DBUS_OBJECT_SKELETON (object));
      g_object_unref (object);
    }

  composition_kit_service_complete_add_surface (service, invocation);
}

static gboolean
on_handle_list_surfaces (CompositionKitService *service,
			 GDBusMethodInvocation *invocation,
			 gpointer	       *user_data)
{
  CompositionKitServicePrivate *priv = (CompositionKitServicePrivate *) user_data;

  GVariant *surfaces = list_surfaces (priv);

  if (surfaces)
    {
      composition_kit_service_complete_list_surfaces (service, invocation, surfaces);
    }
  else
    {
      g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR,
                                             G_DBUS_ERROR_FAILED,
                                             "No surfaces on server\n");
    }
}

static gboolean
on_handle_remove_surface (CompositionKitService *service,
			  GDBusMethodInvocation *invocation,
			  gint		        id,
			  gpointer		*user_data)
{
  CompositionKitServicePrivate *priv = (CompositionKitServicePrivate *) user_data;

  GList   *list;
  list = g_list_find_custom (priv->surfaces, &id, find_surface_by_id);

  if (list)
    {
      CompositionKitSurfacePrivate *spriv   = (CompositionKitSurfacePrivate *) list->data;

      delete_surface (spriv);      
      priv->surfaces = g_list_remove_link (priv->surfaces, list);
    }

  composition_kit_service_complete_remove_surface (service, invocation);
}

static void
on_bus_acquired (GDBusConnection *connection,
	         const gchar     *name,
	         gpointer	  user_data)
{
  manager = g_dbus_object_manager_server_new ("/com/canonical/CompositionKit");

  /* create a new com.canonical.CompositionKit.Service object at
   * /com/canonical/CompositionKit/Server/N where N is 000.. */

  gchar *s = g_strdup_printf ("/com/canonical/CompositionKit/Server/%03d", 1);
  CompositionKitObjectSkeleton *object = composition_kit_object_skeleton_new (s);
  CompositionKitService        *service = composition_kit_service_skeleton_new ();
  CompositionKitServicePrivate *priv    = g_new0 (CompositionKitServicePrivate, 1);

  composition_kit_object_skeleton_set_service (object, service);
  g_object_unref (service);

  g_free (s);
  s = NULL;

  g_signal_connect (G_OBJECT (service), "handle-add-surface",
		    G_CALLBACK (on_handle_add_surface),
		    (gpointer) priv);

  g_signal_connect (G_OBJECT (service), "handle-remove-surface",
		    G_CALLBACK (on_handle_remove_surface),
		    (gpointer) priv);

  g_signal_connect (G_OBJECT (service), "handle-list-surfaces",
		    G_CALLBACK (on_handle_list_surfaces),
		    (gpointer) priv);

  g_dbus_object_manager_server_export (manager, G_DBUS_OBJECT_SKELETON (object));
  g_object_unref (object);

  g_dbus_object_manager_server_set_connection (manager, connection);
}

static void
on_name_acquired (GDBusConnection *connection,
	          const gchar     *name,
	          gpointer	  user_data)
{
  g_debug ("Acquired name %s\n", name);
}

static void
on_name_lost (GDBusConnection *connection,
	      const gchar     *name,
	      gpointer	      user_data)
{
  g_debug ("Lost name %s\n", name);
}

gint
main (gint argc, gchar **argv)
{
  CompositionKitService       *service;
  GMainLoop		      *loop;
  GError		      *error;
  guint         	      id;

  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);

  id = g_bus_own_name (G_BUS_TYPE_SESSION, "com.canonical.CompositionKit.ObjectManager",
		       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
		       G_BUS_NAME_OWNER_FLAGS_REPLACE,
		       on_bus_acquired,
		       on_name_acquired,
		       on_name_lost,
		       loop,
		       NULL);

  g_main_loop_run (loop);

  g_bus_unown_name (id);
  g_main_loop_unref (loop);

  return 0;
}

