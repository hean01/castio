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

#include "js/js.h"

#define DOMAIN "provider"

static void
_js_plugin_search(js_State *state)
{
  js_provider_t *js;
  js = js_touserdata(state, 0, "instance");

  g_log(DOMAIN, G_LOG_LEVEL_MESSAGE,
	"[%s]: Register of search API function.", js->provider->id);

  /* store search function in the registry */
  js_setregistry(state, "plugin.search");
}


void
js_plugin_init(js_State *state, js_provider_t *instance)
{
  js_newobject(state);
  {
    js_getproperty(state, 0, "prototype");
    js_newuserdata(state, "instance", instance);
    js_newcfunction(state, _js_plugin_search, 0);
    js_defproperty(state, -2, "search", 0);
  }
}

