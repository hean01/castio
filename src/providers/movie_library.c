/*
 * This file is part of cast.io
 *
 * Copyright 2014-2015 Henrik Andersson <henrik.4e@gmail.com>
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


#include "service.h"
#include "settings.h"
#include "movie_library.h"

#define DOMAIN "provider.movies"

static JsonNode *
_movie_library_items(cio_provider_descriptor_t *provider,
		     const char *path, gsize offset, gssize limit)
{
  GDir *dir;
  GError *err;
  JsonNode *node;
  JsonObject *object;
  JsonObject *metadata;
  JsonArray *array;

  const gchar *fn;
  gchar fpath[4096];

  err = NULL;

  if (g_strrstr(path, "../") != NULL)
    return NULL;

  g_snprintf(fpath, sizeof(fpath), "%s/%s", "/tmp", path);
  dir = g_dir_open(fpath, 0, &err);
  if (dir == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to read directory: %s", err->message);
    return NULL;
  }

  node = json_node_alloc();
  array = json_array_new();
  json_node_init_array(node, array);
  while((fn = g_dir_read_name(dir)) != NULL)
  {
    if (g_strcmp0(fn, ".") == 0)
      continue;

    if (g_strcmp0(fn, "..") == 0)
      continue;

    object = json_object_new();
    metadata = json_object_new();
    json_object_set_string_member(object, "uri", fn);

    json_object_set_object_member(object, "metadata", metadata);
    json_object_set_string_member(metadata, "title", fn);

    json_array_add_object_element(array, object);
  }

  return node;
}

static void
_movie_library_destroy(cio_provider_descriptor_t *self)
{
  g_free(self);
}

static void
_movie_library_search(cio_provider_descriptor_t *provider,
		      gchar *keywords,
		      cio_provider_search_on_item_callback_t callback,
		      gpointer user_data)
{
  JsonNode *item;

  item = NULL;

  /* TODO: Search for keywords and push each item back through
     callback, use a thread for the searcher */

  /* create two test item_ref and push to caller */
  item = json_node_alloc();
  item = json_node_init_string(item, "/provider/movies/item/91728277");
  callback(provider, item, user_data);

  item = json_node_alloc();
  item = json_node_init_string(item, "/provider/movies/item/82927711");
  callback(provider, item, user_data);

  /* end the search by pushing a NULL item */
  callback(provider, NULL, user_data);
}

static cio_provider_descriptor_t
_movie_library_descriptor =
{
  /* properties */
  .id = "movies",
  .name = "Movie Library",
  .description = "Provides movies from a local storage, scans and updates movie items with metadata from themoviedb.org.",
  .copyright = "2014 - CAST.IO Development Team",
  .version = {0,0,1},
  .homepage = "http://github.com/hean01/castio",
  .icon = "http://raw.githubusercontent.com/hean01/castio/master/images/movie_library_provider.png",

  .destroy = _movie_library_destroy,

  /* api */
  .items = _movie_library_items,
  .search = _movie_library_search,
};

cio_provider_descriptor_t *
cio_provider_movie_library_new(cio_service_t *service)
{
  cio_provider_descriptor_t *desc;
  JsonNode *value;

  desc = g_malloc(sizeof(cio_provider_descriptor_t));
  *desc = _movie_library_descriptor;

  /* create default plugin settings */
  value = json_node_alloc();
  value = json_node_init_string(value, "/storage/movies");
  cio_settings_create_value(service->settings, desc->id, "path", "Library path",
			    "The storage path for the movie library.", value, NULL);

  return desc;
}
