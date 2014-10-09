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

#ifndef _service_h
#define _service_h

#include <json-glib/json-glib.h>

typedef struct cio_service_t
{
  struct cio_service_priv_t *priv;
  struct cio_settings_t *settings;
  struct cio_search_t *search;
  GHashTable *providers;
} cio_service_t;

struct cio_service_t *cio_service_new();

void cio_service_destroy(struct cio_service_t *self);

gboolean cio_service_initialize(struct cio_service_t *self,
				int argc, char **argv);


void cio_service_main(struct cio_service_t *self);

void cio_service_quit(struct cio_service_t *self);


#endif /* _service_h */
