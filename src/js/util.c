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

#include <js/js.h>

void
js_util_pushjsonnode(js_State *state, JsonNode *node)
{
  GType type;

  if (!JSON_NODE_HOLDS_VALUE(node))
  {
    js_pushundefined(state);
    return;
  }

  type = json_node_get_value_type(node);
  if (type == G_TYPE_STRING)
    js_pushstring(state, json_node_get_string(node));
  else if (type == G_TYPE_INT)
    js_pushnumber(state, json_node_get_int(node));
  else if (type == G_TYPE_BOOLEAN)
    js_pushboolean(state, json_node_get_boolean(node));
  else
    js_pushundefined(state);
}

JsonNode *
js_util_tojsonnode(js_State *state, int idx)
{
  JsonNode *node;
  node = json_node_alloc();

  if (js_isstring(state, idx))
    json_node_init_string(node, g_strdup(js_tostring(state, idx)));
  else if (js_isnumber(state, idx))
    json_node_init_int(node, js_tointeger(state, idx));
  else if (js_isboolean(state, idx))
    json_node_init_boolean(node, js_toboolean(state, idx));
  else
  {
    json_node_free(node);
    return NULL;
  }

  return node;
}