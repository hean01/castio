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

#include <archive.h>
#include <archive_entry.h>
#include <string.h>
#include <glib.h>

#include <json-glib/json-glib.h>

#include <mujs/mujs.h>

#include "service.h"
#include "settings.h"
#include "provider.h"
#include "providers/movie_library.h"

struct JSRuntime *_provider_js_runtime;

typedef struct _provider_js_t
{
  js_State *state;
  cio_provider_descriptor_t *provider;
} _provider_js_t;

static cio_provider_descriptor_t *
_provider_js_plugin_manifest_parse(gchar *manifest, gssize len, gchar **icon, gchar **plugin)
{
  GError *err;
  JsonParser *parser;
  JsonNode *node;
  JsonObject *object;
  cio_provider_descriptor_t *provider;

  provider = NULL;
  err = NULL;

  parser = json_parser_new();
  if (!json_parser_load_from_data(parser, manifest, len, &err))
  {
    g_warning("Failed to parse plugin manifest: %s", err->message);
    return NULL;
  }

  node = json_parser_get_root(parser);
  if (!JSON_NODE_HOLDS_OBJECT(node))
  {
    g_warning("Failed to parse plugin manifest: Not a json object.");
    return NULL;
  }

  /* get values from manifest */
  object = json_node_get_object(node);
  provider = g_malloc(sizeof(cio_provider_descriptor_t));
  memset(provider, 0, sizeof(cio_provider_descriptor_t));

  if (!json_object_has_member(object, "id")
      || !json_object_has_member(object, "name")
      || !json_object_has_member(object, "description")
      || !json_object_has_member(object, "copyright")
      || !json_object_has_member(object, "version")
      || !json_object_has_member(object, "url")
      || !json_object_has_member(object, "icon")
      || !json_object_has_member(object, "plugin"))
  {
    g_warning("Failed to parse plugin manifest: Manifest is missing a required member.");
    return NULL;
  }

  provider->id = g_strdup(json_object_get_string_member(object, "id"));
  provider->name = g_strdup(json_object_get_string_member(object, "name"));
  provider->description = g_strdup(json_object_get_string_member(object, "description"));
  provider->copyright = g_strdup(json_object_get_string_member(object, "copyright"));
  provider->homepage = g_strdup(json_object_get_string_member(object, "url"));
  *icon = g_strdup(json_object_get_string_member(object, "icon"));
  *plugin = g_strdup(json_object_get_string_member(object, "plugin"));

  g_object_unref(parser);

  return provider;
}

static void
_js_service_log(js_State *state)
{
  _provider_js_t *js;
  const char *message;

  js = js_touserdata(state, 0, "instance");

  message = js_tostring(state, 1);
  g_warning("%s: %s", js->provider->id, message);
  js_pushundefined(state);
}

static void
_js_plugin_search(js_State *state)
{
  _provider_js_t *js;
  js = js_touserdata(state, 0, "instance");

  g_warning("%s: Register of search API function.", js->provider->id);

  /* store search function in the registry */
  js_setregistry(state, "plugin.search");
}

static JsonNode *
_js_tojsonnode(js_State *state, int idx)
{
  JsonNode *node;

  node = json_node_alloc();

  if (js_isstring(state, 4))
    json_node_init_string(node, g_strdup(js_tostring(state,4)));
  else if (js_isnumber(state, 4))
    json_node_init_int(node, js_tointeger(state,4));
  else if (js_isboolean(state, 4))
    json_node_init_boolean(node, js_toboolean(state,4));
  else
  {
    json_node_free(node);
    return NULL;
  }

  return node;
}

static void
_js_pushjsonnode(js_State *state, JsonNode *node)
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

static void
_js_settings_define(js_State *state)
{
  JsonNode *value;
  _provider_js_t *js;
  const char *id;
  const char *name;
  const char *description;

  js = js_touserdata(state, 0, "instance");
  id = js_tostring(state, 1);
  name = js_tostring(state, 2);
  description = js_tostring(state, 3);

  value = _js_tojsonnode(state, 4);
  if (value == NULL)
  {
    g_warning("%s: Tried to create setting '%s' with unsupported value type.",
	      js->provider->id, id);
    js_pushundefined(state);
  }

  cio_settings_create_value(js->provider->service->settings,
			    js->provider->id,
			    id, name, description,
			    value, NULL);

  /* return value */
  js_pushundefined(state);
}

static void
_js_settings_get(js_State *state)
{
  JsonNode *value;
  _provider_js_t *js;
  const char *id;
  js = js_touserdata(state, 0, "instance");
  id = js_tostring(state, 1);

  value = cio_settings_get_value(js->provider->service->settings,
				 js->provider->id, id, NULL);

  /* return value */
  _js_pushjsonnode(state, value);
}

static gboolean
_provider_js_plugin_init(cio_provider_descriptor_t *provider, gchar *content, gssize len)
{
  _provider_js_t *js;

  provider->opaque = NULL;

  /* initialize JS context for plugin */
  js = g_malloc(sizeof(_provider_js_t));
  memset(js, 0, sizeof(_provider_js_t));
  js->provider = provider;
  js->state = js_newstate(NULL, NULL);

  /* service object added to global scope */
  js_newobject(js->state);
  {

    /* add user data to service */
    js_getproperty(js->state, 0, "prototype");
    js_newuserdata(js->state, "instance", js);

    js_newcfunction(js->state, _js_service_log, 1);
    js_defproperty(js->state, -2, "log", JS_READONLY);
  }
  js_setglobal(js->state, "service");


  /* service.settings object */
  js_newobject(js->state);
  {
    js_getproperty(js->state, 0, "prototype");
    js_newuserdata(js->state, "instance", js);

    js_newcfunction(js->state, _js_settings_define, 4);
    js_defproperty(js->state, -2, "define", JS_READONLY);

    js_newcfunction(js->state, _js_settings_get, 1);
    js_defproperty(js->state, -2, "get", JS_READONLY);
  }
  js_setglobal(js->state, "settings");


  /* create plugin object */
  js_newobject(js->state);
  js_getproperty(js->state, 0, "prototype");
  js_newuserdata(js->state, "instance", js);
  js_newcfunction(js->state, _js_plugin_search, 0);
  js_defproperty(js->state, -2, "search", 0);
  js_setglobal(js->state, "plugin");

  /* read and parse the javascript */
  if (js_ploadstring(js->state, "filename.js", content) != 0)
  {
    g_warning("Failed to parse script.");
    return FALSE;
  }
  js_newobject(js->state);
  js_call(js->state, 0);

  provider->opaque = js;

  g_warning("Evaluate javascript for provider '%s' done.", provider->name);

  return TRUE;
}

static  void _provider_js_plugin_search_proxy(struct cio_provider_descriptor_t *self,
					      gchar *keywords,
					      cio_provider_search_on_item_callback_t callback,
					      gpointer user_data)
{
  int idx;
  char **kw, **pkw;
  _provider_js_t *js;

  js = self->opaque;

  /* push functiona and this object to stack  */
  js_getregistry(js->state, "plugin.search");
  js_newobject(js->state);

  /* push arg keywords array to stack  */
  idx = 0;
  js_newarray(js->state);
  pkw = kw = g_strsplit(keywords, ",", -1);
  for (; *pkw != NULL; pkw++) {
    js_pushstring(js->state, *pkw);
    js_setindex(js->state, -1, idx++);
  }
  js_setlength(js->state, -1, idx);
  g_strfreev(kw);

  /* perform the function call */
  js_call(js->state, 1);
}

static cio_provider_descriptor_t *
_provider_js_plugin_instance(cio_service_t *service, const gchar *filename)
{
  int res;
  gssize len;
  gchar *content;
  gchar *icon, *plugin;
  struct archive *ar;
  struct archive_entry *entry;
  cio_provider_descriptor_t *provider;

  g_assert(g_file_test(filename, G_FILE_TEST_EXISTS));

  provider = NULL;
  plugin = icon = NULL;

  /* read manifest and javascript of plugin */
  ar = archive_read_new();
  archive_read_set_format(ar, ARCHIVE_FORMAT_ZIP);
  res = archive_read_open_filename(ar, filename, 10240);
  if (res != ARCHIVE_OK)
  {
    g_warning("Failed to open archive '%s' with reason: %s",
	      filename, archive_error_string(ar));
    return NULL;
  }

  while(1)
  {
    res = archive_read_next_header(ar, &entry);
    if (res == ARCHIVE_EOF)
      break;

    if (res != ARCHIVE_OK)
      break;

    /* read and parse manifest */
    if (strcmp(archive_entry_pathname(entry), "manifest") == 0)
    {
      len = archive_entry_size(entry);
      content = g_malloc(len + 1);
      memset(content, 0, len + 1);
      archive_read_data(ar, content, len);
      provider = _provider_js_plugin_manifest_parse(content, len, &icon, &plugin);
      g_free(content);
      if (provider == NULL)
      {
	archive_read_close(ar);
	archive_read_free(ar);
	return NULL;
      }

      g_warning("Provider plugin manifest for '%s' script '%s' loaded.", provider->name, plugin);
    }
  }

  archive_read_close(ar);
  archive_read_free(ar);

  /* setup provider proxy functions */
  provider->service = service;
  provider->search = _provider_js_plugin_search_proxy;

  /* Reopen to read plugin script and icon */
  ar = archive_read_new();
  archive_read_set_format(ar, ARCHIVE_FORMAT_ZIP);
  res = archive_read_open_filename(ar, filename, 10240);
  if (res != ARCHIVE_OK)
  {
    g_warning("Failed to open archive '%s' with reason: %s",
	      filename, archive_error_string(ar));
    return NULL;
  }

  while (1)
  {
    res = archive_read_next_header(ar, &entry);
    if (res == ARCHIVE_EOF)
      break;

    if (res != ARCHIVE_OK)
      break;

    if (plugin && strcmp(archive_entry_pathname(entry), plugin) == 0)
    {
      len = archive_entry_size(entry);
      content = g_malloc(len + 1);
      memset(content, 0, len + 1);
      archive_read_data(ar, content, len);
      res = _provider_js_plugin_init(provider, content, len);
      g_free(content);
      if (res == FALSE)
	return NULL;
    }

  }

  archive_read_close(ar);
  archive_read_free(ar);

  if (provider->opaque == NULL)
  {
    g_warning("No plugin script loaded for plugin '%s'", provider->name);
    g_free(provider);
    return NULL;
  }

  return provider;
}

cio_provider_descriptor_t *
cio_provider_instance(cio_service_t *service, cio_provider_type_t type, const gchar *args)
{
  cio_provider_descriptor_t *provider;

  /* get provider instance */
  switch(type)
  {
  case CIO_PROVIDER_MOVIE_LIBRARY:
    provider = cio_provider_movie_library_new(service);
    break;

  case CIO_PROVIDER_JAVASCRIPT_PLUGIN:
    provider = _provider_js_plugin_instance(service, args);
    break;

  default:
    g_warning("Unhandled provider type %d", type);
    return NULL;
  }

  if (provider)
  {
    provider->service = service;
  }

  return provider;
}

void
cio_provider_request_handler(SoupServer *server, SoupMessage *msg, const char *path,
			     GHashTable *query, SoupClientContext *client,
			     gpointer user_data)
{
  gchar *spath;
  gchar **components;
  static gchar *content = "{}";
  cio_service_t *service;
  cio_provider_descriptor_t *provider;
  JsonGenerator *generator;
  JsonArray *result;
  JsonNode *root;

  service = (cio_service_t *)user_data;

  /* this handler only supports GET methods */
  if (msg->method != SOUP_METHOD_GET)
  {
    soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
    return;
  }

  /* lookup provider by id */
  components = g_strsplit(path, "/", -1);
  provider = g_hash_table_lookup(service->providers, components[2]);
  if (provider == NULL)
  {
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  /* bail out if provider doesn't support items */
  if (provider->items == NULL)
  {
    g_warning("Provider '%s' does not provide items.", provider->id);
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  /* get items from provider */
  spath = g_strjoinv("/", components + 3);
  result = provider->items(spath);
  if (result)
  {
    /* convert result into json text */
    root = json_node_alloc();
    root = json_node_init_array(root, result);
    generator = json_generator_new();
    json_generator_set_pretty(generator, TRUE);
    json_generator_set_root(generator, root);
    content = json_generator_to_data(generator, NULL);
    g_object_unref(generator);
    json_node_free(root);

    /* send result */
    soup_message_set_response(msg,
			      "application/json",
			      SOUP_MEMORY_TAKE,
			      content,
			      strlen(content));
    soup_message_set_status(msg, SOUP_STATUS_OK);
  }
  else
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}
