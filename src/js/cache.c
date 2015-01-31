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

#include <string.h>

#include "js/js.h"
#include "service.h"
#include "blobcache.h"

#define DOMAIN "provider"

static void
_js_cache_store(js_State *state)
{
  js_provider_t *js;
  const char *key;
  const char *data;

  js = js_touserdata(state, 0, "instance");
  key = js_tostring(state, 1);
  data = js_tostring(state, 2);

  g_log(DOMAIN, G_LOG_LEVEL_DEBUG,
	"[%s.cache.store]: %s", js->provider->id, key);

  cio_blobcache_store(js->provider->service->blobcache, 0, g_str_hash(key),
		      data, strlen(data));

  js_pushundefined(state);
}

static void
_js_cache_get(js_State *state)
{
  js_provider_t *js;
  const char *key;
  void *content;

  js = js_touserdata(state, 0, "instance");
  key = js_tostring(state, 1);

  g_log(DOMAIN, G_LOG_LEVEL_DEBUG,
	"[%s.cache.get]: %s", js->provider->id, key);

  if (cio_blobcache_get(js->provider->service->blobcache, g_str_hash(key),
			&content, NULL) == 0)
  {
    js_pushstring(state, content);
    g_free(content);
  }
  else
    js_pushnull(state);
}

void
js_cache_init(js_State *state, js_provider_t *instance)
{
  js_newobject(state);
  {
    /* add user data to service */
    js_getproperty(state, 0, "prototype");
    js_newuserdata(state, "instance", instance, NULL);

    js_newcfunction(state, _js_cache_store, "store",  2);
    js_defproperty(state, -2, "store", JS_READONLY);

    js_newcfunction(state, _js_cache_get, "get", 1);
    js_defproperty(state, -2, "get", JS_READONLY);
  }
}
