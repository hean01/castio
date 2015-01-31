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

#include "provider.h"
#include "service.h"
#include "settings.h"

#define DOMAIN "settings"

static void
_js_settings_define(js_State *state)
{
  JsonNode *value;
  js_provider_t *js;
  const char *id;
  const char *name;
  const char *description;

  js = js_touserdata(state, 0, "instance");
  id = js_tostring(state, 1);
  name = js_tostring(state, 2);
  description = js_tostring(state, 3);

  value = js_util_tojsonnode(state, 4);
  if (value == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "[%s.settings.define]: Define setting '%s' failed: unsupported value type",
	  js->provider->id, id);
    js_pushundefined(state);
    return;
  }

  cio_settings_create_value(js->provider->service->settings,
			    js->provider->id,
			    id, name, description,
			    value, NULL);

  json_node_free(value);

  /* return value */
  js_pushundefined(state);
}


static void
_js_settings_get(js_State *state)
{
  JsonNode *value;
  js_provider_t *js;
  const char *id;
  js = js_touserdata(state, 0, "instance");
  id = js_tostring(state, 1);

  value = cio_settings_get_value(js->provider->service->settings,
				 js->provider->id, id, NULL);

  /* return value */
  js_util_pushjsonnode(state, value);
}

void
js_settings_init(js_State *state, js_provider_t *instance)
{
  /* push a settings object on stack */
  js_newobject(state);
  {
    js_getproperty(state, 0, "prototype");
    js_newuserdata(state, "instance", instance, NULL);

    js_newcfunction(state, _js_settings_define, "define", 4);
    js_defproperty(state, -2, "define", JS_READONLY);

    js_newcfunction(state, _js_settings_get, "get", 1);
    js_defproperty(state, -2, "get", JS_READONLY);
  }
}
