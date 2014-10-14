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

#include "js/js.h"
#include "plugin.h"

#define DOMAIN "plugin"

static cio_provider_descriptor_t *
_provider_plugin_manifest_parse(gchar *manifest, gssize len, gchar **icon, gchar **plugin)
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
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to parse plugin manifest: %s", err->message);
    return NULL;
  }

  node = json_parser_get_root(parser);
  if (!JSON_NODE_HOLDS_OBJECT(node))
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to parse plugin manifest: Not a json object.");
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
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to parse plugin manifest: Manifest object is missing required member(s).");
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

static gboolean
_provider_plugin_init(cio_provider_descriptor_t *provider, gchar *content, gssize len)
{
  js_provider_t *js;
  const gchar *message;

  provider->opaque = NULL;

  /* initialize JS context for plugin */
  js = g_malloc(sizeof(js_provider_t));
  memset(js, 0, sizeof(js_provider_t));
  js->provider = provider;
  js->state = js_newstate(NULL, NULL);

  /* service object added to global scope */
  js_service_init(js->state, js);
  js_setglobal(js->state, "service");

  /* settings object */
  js_settings_init(js->state, js);
  js_setglobal(js->state, "settings");

  /* create plugin object */
  js_plugin_init(js->state, js);
  js_setglobal(js->state, "plugin");

  /* read and parse the javascript */
  if (js_ploadstring(js->state, "script.js", content) != 0)
  {
    js_getproperty(js->state, 1, "message");
    message =  js_tostring(js->state, 3);
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "[%s] %s",
	  provider->id, message);

    return FALSE;
  }
  js_newobject(js->state);
  js_call(js->state, 0);

  provider->opaque = js;

  return TRUE;
}

static void
_provider_plugin_search_proxy(struct cio_provider_descriptor_t *self,
			      gchar *keywords,
			      cio_provider_search_on_item_callback_t callback,
			      gpointer user_data)
{
  int idx;
  char **kw, **pkw;
  js_provider_t *js;

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

cio_provider_descriptor_t *
cio_provider_plugin_new(struct cio_service_t *service, const gchar *filename)
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
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to open plugin archive '%s' with reason: %s",
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
      memset(content, 0, len);
      archive_read_data(ar, content, len);
      provider = _provider_plugin_manifest_parse(content, len, &icon, &plugin);
      g_free(content);
      if (provider == NULL)
      {
	archive_read_close(ar);
	archive_read_free(ar);
	return NULL;
      }
    }
  }

  archive_read_close(ar);
  archive_read_free(ar);

  /* setup provider proxy functions */
  provider->service = service;
  provider->search = _provider_plugin_search_proxy;

  /* Reopen to read plugin script and icon */
  ar = archive_read_new();
  archive_read_set_format(ar, ARCHIVE_FORMAT_ZIP);
  res = archive_read_open_filename(ar, filename, 10240);
  if (res != ARCHIVE_OK)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to open plugin archive '%s' with reason: %s",
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
      memset(content, 0, len);
      archive_read_data(ar, content, len);
      res = _provider_plugin_init(provider, content, len);
      g_free(content);
      if (res == FALSE)
	return NULL;
    }

  }

  archive_read_close(ar);
  archive_read_free(ar);

  if (provider->opaque == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to instantiate plugin '%s'", provider->name);
    g_free(provider);
    return NULL;
  }

  return provider;
}
