/*
 * This file is part of cast.io
 *
 * Copyright 2014-2015 Henrik Andersson <henrik.4e@gmail.com>
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
#include "service.h"
#include "blobcache.h"

#define DOMAIN "plugin"

inline gboolean
_provider_plugin_match(const char *l, const char *r, const char **arg)
{
  const gchar *pl, *pr;

  g_assert(l != NULL);
  g_assert(r != NULL);

  pl = l;
  pr = r;

  while (*pl)
  {
    if (*pl == '*')
    {
      *arg = pr;
      return TRUE;
    }

    if (*pl != *pr)
      break;

    pl++;
    pr++;
  }

  if (*pl == '\0' && *pr == '\0')
    return TRUE;

  return FALSE;
}

static const gchar *
_provider_plugin_match_handler(js_provider_t *js, const char *path, const gchar **arg)
{
  GList *it;

  it = g_list_first(js->paths);
  *arg = NULL;
  while(it)
  {
    if (_provider_plugin_match(it->data, path, arg) == TRUE)
      return it->data;

    it = g_list_next(it);
  }

  return NULL;
}

static cio_provider_descriptor_t *
_provider_plugin_manifest_parse(gchar *manifest, gssize len, gchar **icon, gchar **plugin)
{
  GError *err;
  JsonParser *parser;
  JsonNode *node;
  JsonObject *object;
  JsonArray *version;
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

  /* get version from object and verify that it is a JsonArray of
     three integers */
  version = json_object_get_array_member(object, "version");
  if (!version
      || json_array_get_length(version) != 3
      || !g_type_is_a(json_node_get_value_type(json_array_get_element(version, 0)), G_TYPE_INT64)
      || !g_type_is_a(json_node_get_value_type(json_array_get_element(version, 1)), G_TYPE_INT64)
      || !g_type_is_a(json_node_get_value_type(json_array_get_element(version, 2)), G_TYPE_INT64)
	  )
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to parse plugin manifest: version specified is not an array of three integers.");
    return NULL;
  }

  /* store version into provider descriptor */
  provider->version[0] = json_array_get_int_element(version, 0);
  provider->version[1] = json_array_get_int_element(version, 1);
  provider->version[2] = json_array_get_int_element(version, 2);

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
  js->state = js_newstate(NULL, NULL, JS_STRICT);

  /* service object added to global scope */
  js_service_init(js->state, js);
  js_setglobal(js->state, "service");

  /* blob cache object */
  js_cache_init(js->state, js);
  js_setglobal(js->state, "cache");

  /* settings object */
  js_settings_init(js->state, js);
  js_setglobal(js->state, "settings");

  /* create plugin object */
  js_plugin_init(js->state, js);
  js_setglobal(js->state, "plugin");

  /* add http object */
  js_http_init(js->state, js);
  js_setglobal(js->state, "http");


  /* read and parse the javascript */
  if (js_ploadstring(js->state, "script.js", content) != 0)
  {
    message =  js_tostring(js->state, -1);
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "[%s] %s",
	  provider->id, message);

    return FALSE;
  }

  js_newobject(js->state);
  if (js_pcall(js->state, 0) != 0)
  {
    message = js_tostring(js->state, -1);
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "[%s] Failed to initialize: %s",
	  provider->id, message);
    return FALSE;
  }

  provider->opaque = js;

  return TRUE;
}

static void
_provider_plugin_destroy(struct cio_provider_descriptor_t *self)
{
  js_provider_t *js;

  js = self->opaque;

  js_freestate(js->state);
  g_list_free_full(js->paths, g_free);
  g_free(js);

  g_free(self->id);
  g_free(self->name);
  g_free(self->description);
  g_free(self->copyright);
  g_free(self->homepage);

  g_free(self);
}

static void
_provider_plugin_search_proxy(struct cio_provider_descriptor_t *self,
			      gchar *keywords,
			      cio_provider_search_on_item_callback_t callback,
			      gpointer user_data)
{
  int idx;
  GList *l, *it;
  const gchar *message;
  char **kw, **pkw;
  js_provider_t *js;
  JsonNode *node;
  JsonArray *array;

  js = self->opaque;
  kw = NULL;

  /* push function and this object to stack  */
  js_getregistry(js->state, "plugin.search");
  if (js_isundefined(js->state, -1))
  {
    g_log(DOMAIN, G_LOG_LEVEL_INFO,
	  "[%s.search] Plugin do not support search.", self->id);
    goto bail_out;
  }

  /* push this object to stack */
  js_newobject(js->state);

  /* push arg keywords array to stack  */
  idx = 0;
  js_newarray(js->state);
  pkw = kw = g_strsplit(keywords, "+", -1);
  for (; *pkw != NULL; pkw++) {
    js_pushstring(js->state, *pkw);
    js_setindex(js->state, -2, idx++);
  }
  js_setlength(js->state, -1, idx);

  /* push limit number to stack */
  js_pushnumber(js->state, 10);

  /* perform the function call */
  if (js_pcall(js->state, 2) != 0)
  {
    message =  js_tostring(js->state, -1);
    g_log(DOMAIN, G_LOG_LEVEL_CRITICAL,
	  "[%s.search] %s", self->id, message);
    goto bail_out;
  }

  /* get and process plugin search result */
  if (!js_isarray(js->state, -1))
  {
    g_log(DOMAIN, G_LOG_LEVEL_CRITICAL,
	  "[%s.search] result is not an array.", self->id);
    goto bail_out;
  }

  node = js_util_tojsonnode(js->state, -1);
  array = json_node_get_array(node);

  l = it = json_array_get_elements(array);
  while (it)
  {
    callback(js->provider, it->data, user_data);
    it = g_list_next(it);
  }

  if (l)
    g_list_free(l);

bail_out:
  /* end search for provider */
  callback(js->provider, NULL, user_data);
  if (kw)
    g_strfreev(kw);
}


static JsonNode *
_provider_plugin_items_proxy(struct cio_provider_descriptor_t *self, const gchar *path,
			     gsize offset, gssize limit)
{
  gchar *fp;
  const gchar *arg;
  const gchar *handler;
  JsonNode *node;
  const gchar *message;
  js_provider_t *js;

  js = self->opaque;

  fp = g_malloc(strlen(path) + 2);
  fp[0] = 0;
  g_snprintf(fp, strlen(path) + 2, "/%s", path);

  g_log(DOMAIN, G_LOG_LEVEL_DEBUG,
	"[%s.items] Fetching items for uri '%s'", self->id, fp);

  /* do have have a path match */
  handler = _provider_plugin_match_handler(js, fp, &arg);
  if (handler == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "[%s.items] No matching handler for path '%s' found",
	  self->id, fp);
    return NULL;
  }

  /* fetch function from registry to stack  */
  js_getregistry(js->state, handler);
  if (!js_isdefined(js->state, -1))
  {
    g_log(DOMAIN, G_LOG_LEVEL_CRITICAL,
	  "[%s.items] Handler function '%s' not found", self->id, handler);
    js_pop(js->state, 1);
    return NULL;
  }

  /* add this object to stack */
  js_newobject(js->state);

  /* push arg: offset to stack */
  js_newnumber(js->state, offset);

  /* push arg: limit to stack */
  js_newnumber(js->state, limit);

  /* push arg: match arg to stack */
  if (arg)
    js_newstring(js->state, arg);

  /* perform the function call */
  if (js_pcall(js->state, (arg != NULL) ? 3 : 2) != 0)
  {
    message =  js_tostring(js->state, -1);
    g_log(DOMAIN, G_LOG_LEVEL_CRITICAL,
	  "[%s.items] %s", self->id, message);
  }

  /* get result array */
  node = js_util_tojsonnode(js->state, -1);
  return node;
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

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"Creating instance of: %s", filename);

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
	goto cleanup;
    }
  }

  if (provider == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Provider plugin '%s' does not contain a manifest.", filename);
    return NULL;
  }

  archive_read_close(ar);
  archive_read_free(ar);

  /* setup provider proxy functions */
  provider->service = service;
  provider->destroy = _provider_plugin_destroy;
  provider->search = _provider_plugin_search_proxy;
  provider->items = _provider_plugin_items_proxy;

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

    /* read the provider script */
    if (plugin && strcmp(archive_entry_pathname(entry), plugin) == 0)
    {
      len = archive_entry_size(entry);
      content = g_malloc(len + 1);
      memset(content, 0, len + 1);
      archive_read_data(ar, content, len);
      res = _provider_plugin_init(provider, content, len);
      g_free(content);
      if (res == FALSE)
      {
	g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	      "Provider '%s' failed to initialize.", filename);
	goto cleanup;
      }
    }

    /* read the icon and store into blobcache with mimetype + size header */
    if (plugin && strcmp(archive_entry_pathname(entry), icon) == 0)
    {
      char uri[512];
      cio_blobcache_resource_header_t hdr;
      memset(&hdr, 0, sizeof(hdr));
      snprintf(hdr.mime, sizeof(hdr.mime), "image/png");
      hdr.size = archive_entry_size(entry);

      content = malloc(hdr.size + sizeof(hdr));
      memset(content, 0, hdr.size + sizeof(hdr));
      memcpy(content, &hdr, sizeof(hdr));

      archive_read_data(ar, content + sizeof(hdr), hdr.size);

      // store icon to blob cache
      snprintf(uri, sizeof(uri), "%s://%s", provider->id, icon);
      provider->icon = g_strdup(uri);
      cio_blobcache_store(service->blobcache, 0, g_str_hash(uri), content, hdr.size + sizeof(hdr));
    }

  }

  if (provider->opaque == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "[%s] JavaScript plugin specified by manifest was not found.", provider->id);
  }


cleanup:
  archive_read_close(ar);
  archive_read_free(ar);

  g_free(icon);
  g_free(plugin);

  if (provider == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to load plugin '%s'", filename);
    g_free(provider);
    return NULL;
  }

  if (provider->opaque == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "[%s] failed to create instance of plugin", provider->id);
    g_free(provider);
    return NULL;
  }

  return provider;
}
