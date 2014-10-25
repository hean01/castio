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

#ifndef _settings_h
#define _settings_h

#include <glib.h>
#include <json-glib/json-glib.h>
#include <libsoup/soup.h>

struct cio_settings_t *settings;

struct cio_settings_t *cio_settings_new();
void cio_settings_destroy(struct cio_settings_t *self);

gboolean cio_settings_load(struct cio_settings_t *self, GError **err);
gboolean cio_settings_save(struct cio_settings_t *self, GError **err);

gboolean cio_settings_has_section(struct cio_settings_t *self,
				  const char *section);

gboolean cio_settings_update_section(struct cio_settings_t *self,
				     const char *section,
				     JsonNode *node);

gboolean cio_settings_has_value(struct cio_settings_t *self,
				const char *section,
				const char *id);

JsonNode *cio_settings_get_value(struct cio_settings_t *self,
				 const gchar *section,
				 const gchar *id,
				 GError **err);

gboolean cio_settings_create_value(struct cio_settings_t *self,
				   const char *section,
				   const char *id,
				   const char *name,
				   const char *description,
				   JsonNode *value,
				   GError **err);

char *cio_settings_get_string_value(struct cio_settings_t *self,
				    const char *section,
				    const char *id,
				    GError **err);

int cio_settings_get_int_value(struct cio_settings_t *self,
			       const char *section,
			       const char *id,
			       GError **err);

gboolean cio_settings_get_boolean_value(struct cio_settings_t *self,
					const char *section,
					const char *id,
					GError **err);

void cio_settings_request_handler(SoupServer *server, SoupMessage *msg,
				  const char *path, GHashTable *query,
				  SoupClientContext *client, gpointer user_data);

#endif /* _settings.h */
