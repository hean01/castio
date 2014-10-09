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

#ifndef _provider_h
#define _provider_h

#include <glib.h>
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>

struct cio_service_t;
struct cio_provider_descriptor_t;

typedef void (*cio_provider_search_on_item_callback_t)(struct cio_provider_descriptor_t *self,
						       JsonNode *item, gpointer user_data);

typedef enum cio_provider_type_t
{
  CIO_PROVIDER_MOVIE_LIBRARY,
  CIO_PROVIDER_JAVASCRIPT_PLUGIN
} cio_provider_type_t;

typedef struct cio_provider_descriptor_t
{
  /*
   * properties
   */
  gchar *id;
  gchar *name;
  gchar *description;
  gchar *copyright;
  gchar *version;
  gchar *homepage;
  gchar *icon;

  /*
   * api
   */

  /** get a list of items for specified path */
  JsonArray *(*items)(const char *path);

  void (*search)(struct cio_provider_descriptor_t *self,
		 gchar *keywords,
		 cio_provider_search_on_item_callback_t callback,
		 gpointer user_data);
} cio_provider_descriptor_t;

cio_provider_descriptor_t *cio_provider_instance(struct cio_service_t *service,
						 cio_provider_type_t type,
						 const gchar *args);

void cio_provider_request_handler(SoupServer *server, SoupMessage *msg, const char *path,
				  GHashTable *query, SoupClientContext *client, gpointer user_data);
#endif /* _provider_h */
