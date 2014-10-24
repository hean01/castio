/*
 * This file is part of cast.io
 *
 * Copyright 2014 Henrik Andersson <henrik.4e@gmail.com>
 *
 * cast.io is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cast.io is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cast.io.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <archive.h>
#include <archive_entry.h>
#include <string.h>
#include <glib.h>

#include <json-glib/json-glib.h>

#include "service.h"
#include "settings.h"
#include "provider.h"
#include "providers/plugin.h"
#include "providers/movie_library.h"

#define DOMAIN "provider"

cio_provider_descriptor_t *
cio_provider_instance(cio_service_t *service, cio_provider_type_t type, const gchar *args)
{
  JsonNode *value;
  cio_provider_descriptor_t *provider;

  /* get provider instance */
  switch(type)
  {
  case CIO_PROVIDER_MOVIE_LIBRARY:
    provider = cio_provider_movie_library_new(service);
    break;

  case CIO_PROVIDER_JAVASCRIPT_PLUGIN:
    provider = cio_provider_plugin_new(service, args);
    break;

  default:
    return NULL;
  }

  if (provider == NULL)
    return NULL;


  provider->service = service;

  /* add provider setting 'enabled' if not exists */
  if (!cio_settings_has_value(service->settings,
			      provider->id, "enabled"))
  {
    value = json_node_init_boolean(json_node_alloc(), TRUE);
    cio_settings_create_value(service->settings,
			      provider->id, "enabled",
			      "Enabled",
			      "Toggle if provider should be enabled of not.",
			      value, NULL);
  }

  return provider;
}

void
cio_provider_request_handler(SoupServer *server, SoupMessage *msg, const char *path,
			     GHashTable *query, SoupClientContext *client,
			     gpointer user_data)
{
  gchar *spath;
  gchar **components;
  static gchar *content = "{}";
  cio_service_t *service;
  cio_provider_descriptor_t *provider;
  JsonGenerator *generator;
  JsonNode *result;

  service = (cio_service_t *)user_data;

  /* this handler only supports GET methods */
  if (msg->method != SOUP_METHOD_GET)
  {
    soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
    return;
  }

  /* lookup provider by id */
  components = g_strsplit(path, "/", -1);
  provider = g_hash_table_lookup(service->providers, components[2]);
  if (provider == NULL)
  {
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  /* bail out if provider doesn't support items */
  if (provider->items == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Provider '%s' does not provide items.", provider->id);
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  /* get items from provider */
  spath = g_strjoinv("/", components + 3);
  result = provider->items(provider, spath, 0, 10);
  if (result)
  {
    /* convert result into json text */
    generator = json_generator_new();
    json_generator_set_pretty(generator, TRUE);
    json_generator_set_root(generator, result);
    content = json_generator_to_data(generator, NULL);
    g_object_unref(generator);
    json_node_free(result);

    /* send result */
    soup_message_set_response(msg,
			      "application/json; charset=utf-8",
			      SOUP_MEMORY_TAKE,
			      content,
			      strlen(content));
    soup_message_set_status(msg, SOUP_STATUS_OK);
  }
  else
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}
