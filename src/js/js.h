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

#ifndef _js_h
#define _js_h

#include <json-glib/json-glib.h>

#include "mujs/mujs.h"
#include "provider.h"

typedef struct js_provider_t
{
  js_State *state;
  GList *paths;
  cio_provider_descriptor_t *provider;
} js_provider_t;

void js_service_init(js_State *state, js_provider_t *instance);
void js_settings_init(js_State *state, js_provider_t *instance);
void js_plugin_init(js_State *state, js_provider_t *instance);
void js_http_init(js_State *state, js_provider_t *instance);
void js_cache_init(js_State *state, js_provider_t *instance);

/* util */
void js_util_pushjsonnode(js_State *state, JsonNode *node);
JsonNode *js_util_tojsonnode(js_State *state, int idx);

#endif /* _js_h */
