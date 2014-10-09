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

#ifndef _search_h
#define _search_h

#include <glib.h>
#include <libsoup/soup.h>

struct cio_search_t;

struct cio_search_t *cio_search_new();

void cio_search_destroy(struct cio_search_t *self);

void cio_search_request_handler(SoupServer *server, SoupMessage *msg,
				const char *path, GHashTable *query,
				SoupClientContext *client, gpointer user_data);

#endif /* _search_h */
