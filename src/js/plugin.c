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

#define DOMAIN "provider"

static void
_js_plugin_search(js_State *state)
{
  js_provider_t *js;
  js = js_touserdata(state, 0, "instance");

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"[%s.plugin.search]: register handler function for search.", js->provider->id);

  /* store search function in the registry */
  js_setregistry(state, "plugin.search");

  /* return value */
  js_pushundefined(state);
}

static void
_js_plugin_register(js_State *state)
{
  const gchar *uri;
  js_provider_t *js;

  js = js_touserdata(state, 0, "instance");
  uri = js_tostring(state, 1);

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"[%s.plugin.register]: register enumeration uri '%s'", js->provider->id, uri);

  /* verify that wildcard is used correctly */
  if (strrchr(uri, '*') != NULL && uri[strlen(uri) - 1] != '*')
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "[%s.plugin.register]: wildcard is not at the end of uri", js->provider->id);
    js_pushundefined(state);
    return;
  }

  /* add uri to paths match */
  js->paths = g_list_append(js->paths, g_strdup(uri));

  /* store uri in the registry */
  js_setregistry(state, uri);

  /* return value */
  js_pushundefined(state);
}


void
js_plugin_init(js_State *state, js_provider_t *instance)
{
  gchar uri_prefix[512];
  g_snprintf(uri_prefix, sizeof(uri_prefix), "/providers/%s", instance->provider->id);

  js_newobject(state);
  {
    js_getproperty(state, 0, "prototype");
    js_newuserdata(state, "instance", instance, NULL);

    js_pushstring(state, uri_prefix);
    js_defproperty(state, -2, "URI_PREFIX", JS_READONLY);

    js_newobject(state);
    {
      js_pushstring(state, "folder");
      js_defproperty(state, -2, "TYPE_FOLDER", JS_READONLY);
      js_pushstring(state, "radiostation");
      js_defproperty(state, -2, "TYPE_RADIO_STATION", JS_READONLY);
      js_pushstring(state, "musictrack");
      js_defproperty(state, -2, "TYPE_MUSIC_TRACK", JS_READONLY);
      js_pushstring(state, "video");
      js_defproperty(state, -2, "TYPE_VIDEO", JS_READONLY);
      js_pushstring(state, "movie");
      js_defproperty(state, -2, "TYPE_MOVIE", JS_READONLY);
      js_pushstring(state, "tvserie");
      js_defproperty(state, -2, "TYPE_TVSERIE", JS_READONLY);
     }
    js_defproperty(state, -2, "item", JS_READONLY);

    js_newcfunction(state, _js_plugin_search, "search", 0);
    js_defproperty(state, -2, "search", 0);
    js_newcfunction(state, _js_plugin_register, "register", 0);
    js_defproperty(state, -2, "register", 0);

  }
}

