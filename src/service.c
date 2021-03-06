/*
 * This file is part of cast.io
 *
 * Copyright 2014-2018 Henrik Andersson <henrik.4e@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <glib.h>

#include "config.h"
#include "blobcache.h"
#include "service.h"
#include "search.h"
#include "settings.h"
#include "provider.h"

#define DOMAIN "service"

#define AUTH_REALM "CAST.IO"

static gchar *g_option_config_file = CASTIO_INSTALL_PREFIX"/etc/castio.conf";

static GOptionEntry entries[] =
{
  {"config", 'c', 0, G_OPTION_ARG_STRING, &g_option_config_file, "Use specified config file instead of default", NULL},
  { NULL }
};

typedef struct cio_service_priv_t
{
  GMainLoop *loop;
  gchar *config_file;
  SoupServer *server;
  SoupAuthDomain *domain;
  GQueue *backlog;
} cio_service_priv_t;

static JsonNode *
_service_log_entry(const char *timestamp,
		   const gchar *log_domain,
		   GLogLevelFlags log_level,
		   const gchar *message)
{
  gchar *level;
  JsonNode *node;
  JsonObject *object;
  node = json_node_alloc();
  object = json_object_new();
  json_node_take_object(node, object);

  json_object_set_string_member(object, "timestamp", timestamp);
  json_object_set_string_member(object, "domain", log_domain);

  level = "undefined";
  if (log_level & G_LOG_LEVEL_ERROR)
    level = "error";
  else if (log_level & G_LOG_LEVEL_CRITICAL)
    level = "critical";
  else if (log_level & G_LOG_LEVEL_WARNING)
    level = "warning";
  if (log_level & G_LOG_LEVEL_MESSAGE)
    level = "message";
  else if (log_level & G_LOG_LEVEL_INFO)
    level = "info";
  else if (log_level &  G_LOG_LEVEL_DEBUG)
    level = "debug";

  json_object_set_string_member(object, "level", level);
  json_object_set_string_member(object, "message", message);

  return node;
}

static void
_service_log_handler(const gchar *log_domain,
		     GLogLevelFlags log_level,
		     const gchar *message,
		     gpointer user_data)
{
  gchar *level;
  FILE *o;
  struct timeval tv;
  gchar timestamp[512];

  cio_service_t *service;
  service = (cio_service_t *)user_data;

  gettimeofday(&tv, NULL);

  g_snprintf(timestamp, sizeof(timestamp), "%d.%d", (int)tv.tv_sec, (int)tv.tv_usec);

  /* add log entry to backlog, pop head item if full */
  g_queue_push_tail(service->priv->backlog,
		    _service_log_entry(timestamp, log_domain, log_level, message));

  if (g_queue_get_length(service->priv->backlog) >= 100)
    json_node_free(g_queue_pop_head(service->priv->backlog));

  /* print the log message */
  if (log_level & (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL
		   | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_INFO))
    o = stderr;
  else
    o = stdout;

  level = "Undefined";
  if (log_level & G_LOG_LEVEL_ERROR)
    level = "ERROR";
  else if (log_level & G_LOG_LEVEL_CRITICAL)
    level = "CRITICAL";
  else if (log_level & G_LOG_LEVEL_WARNING)
    level = "WARNING";
  if (log_level & G_LOG_LEVEL_MESSAGE)
    level = "MESSAGE";
  else if (log_level & G_LOG_LEVEL_INFO)
    level = "INFO";
  else if (log_level &  G_LOG_LEVEL_DEBUG)
    level = "DEBUG";

  //strftime(timestamp, sizeof(timestamp), "%F %T", (time_t*)&tv.tv_sec);
  fprintf(o, "%s: %s: [%s] %s\n",timestamp, level, log_domain, message);

  if (log_level & (G_LOG_FLAG_FATAL) && !(log_level & G_LOG_LEVEL_ERROR))
    abort();
}

/** setup default values for missing keys in configuration */
static void
_service_configuration_defaults(cio_service_t *self)
{
  char *digest;
  JsonNode *value;

  /* intialize listen port */
  value = json_node_alloc();
  value = json_node_init_int(value, 1457);
  cio_settings_create_value(self->settings, "service", "port",
			    "Service port",
			    "Port were the service listens for web api requests."
			    " Requires a restart of the service.",
			    value, NULL);
  json_node_free(value);

  /* intialize plugin directory */
  value = json_node_alloc();
  value = json_node_init_string(value, CASTIO_DATA_DIR"/plugins");
  cio_settings_create_value(self->settings, "service", "plugin_dir",
			    "Plugin directory",
			    "Specifies were the service searchs for plugins to be loaded."
			    " Requires a restart of the service.",
			    value, NULL);
  json_node_free(value);

  /* intialize digest */
  digest = soup_auth_domain_digest_encode_password("admin", AUTH_REALM, "password");
  value = json_node_alloc();
  value = json_node_init_string(value, digest);
  cio_settings_create_value(self->settings, "service", "auth_digest",
			    "Authentication digest",
			    "The encoded authentication digest used for accessing the web api.",
			    value, NULL);
  json_node_free(value);
}

/** Initialize providers internal and plugins */
static void
_service_initialize_providers(cio_service_t *self)
{
  GError *err;
  const gchar *filename;
  gchar *plugindir;
  gchar plugin[4096];
  GDir *dir;
  cio_provider_descriptor_t *provider;

  err = NULL;

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"Initialize of provider plugins.");

  /* instantiate internal providers */
  provider = cio_provider_instance(self, CIO_PROVIDER_MOVIE_LIBRARY, NULL);
  if (provider)
    g_hash_table_insert(self->providers, provider->id, provider);
  else
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to instantiate internal Movie library provider.");

  /* load and instantiate available plugin providers */
  plugindir = cio_settings_get_string_value(self->settings, "service", "plugin_dir", &err);
  if (err)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "No plugin directory specified in configuration.\n");
    g_clear_error(&err);
    return;
  }

  dir = g_dir_open(plugindir, 0, &err);
  if (err)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to load plugins from directory: %s\n", err->message);
    g_free(plugindir);
    g_clear_error(&err);
    return;
  }

  while ((filename = g_dir_read_name(dir)) != NULL)
  {
    if (g_strcmp0(filename, ".") == 0 || g_strcmp0(filename, "..") == 0)
      continue;

    if (g_strcmp0(filename + strlen(filename) - 4, ".zip") != 0)
      continue;

    g_snprintf(plugin, sizeof(plugin), "%s/%s", plugindir, filename);
    provider = cio_provider_instance(self, CIO_PROVIDER_JAVASCRIPT_PLUGIN, plugin);
    if (provider == NULL)
      continue;

    g_hash_table_insert(self->providers, provider->id, provider);
  }

  g_dir_close(dir);
  g_free(plugindir);
}

/** create json array of backlog */
static gchar *
_service_backlog_to_json(cio_service_t *self, gsize *length)
{
  int i, len;
  gchar *content;
  JsonNode *node;
  JsonNode *item;
  JsonArray *array;
  JsonGenerator *gen;

  node = json_node_alloc();
  array = json_array_new();
  json_node_init_array(node, array);

  len = g_queue_get_length(self->priv->backlog);

  for (i = 0; i < len; i++)
  {
    item = g_queue_peek_nth(self->priv->backlog, i);

    if (item == NULL)
      continue;

    json_array_add_element(array, json_node_copy(item));
  }

  /* stringify json node */
  gen = json_generator_new();
  json_generator_set_pretty(gen, TRUE);
  json_generator_set_root(gen, node);
  content = json_generator_to_data(gen, length);
  json_node_free(node);
  g_object_unref(gen);
  return content;
}

/** create json array of available providers */
static gchar *
_service_providers_to_json(cio_service_t *self, gsize *length, gsize offset, gsize limit)
{
  cio_provider_descriptor_t *provider;
  JsonGenerator *gen;
  JsonBuilder *builder;
  JsonNode *root;
  GList *item;
  gchar *content;
  gsize cnt;

  item = g_hash_table_get_values(self->providers);

  builder = json_builder_new();
  builder = json_builder_begin_array(builder);
  cnt = 0;
  while (item)
  {
    /* continue if offset not reached */
    if (offset > cnt)
    {
      cnt++;
      item = g_list_next(item);
      continue;
    }

    /* break out if limit is reached */
    if (cnt >= offset+limit)
      break;

    provider = item->data;

    /* create provider object */
    builder = json_builder_begin_object(builder);

    /* add properties */
    builder = json_builder_set_member_name(builder, "id");
    builder = json_builder_add_string_value(builder, provider->id);
    builder = json_builder_set_member_name(builder, "name");
    builder = json_builder_add_string_value(builder, provider->name);
    builder = json_builder_set_member_name(builder, "description");
    builder = json_builder_add_string_value(builder, provider->description);

    builder = json_builder_set_member_name(builder, "version");
    builder = json_builder_begin_array(builder);
    builder = json_builder_add_int_value(builder, provider->version[0]);
    builder = json_builder_add_int_value(builder, provider->version[1]);
    builder = json_builder_add_int_value(builder, provider->version[2]);
    builder = json_builder_end_array(builder);

    builder = json_builder_set_member_name(builder, "copyright");
    builder = json_builder_add_string_value(builder, provider->copyright);
    builder = json_builder_set_member_name(builder, "homepage");
    builder= json_builder_add_string_value(builder, provider->homepage);
    builder = json_builder_set_member_name(builder, "icon");
    builder= json_builder_add_string_value(builder, provider->icon);

    builder = json_builder_end_object(builder);
    item = g_list_next(item);
    cnt++;
  }
  builder = json_builder_end_array(builder);

  /* convert json object to text representation */
  gen = json_generator_new();
  root = json_builder_get_root(builder);
  json_generator_set_pretty(gen, TRUE);
  json_generator_set_root(gen, root);
  content = json_generator_to_data(gen, length);
  json_node_free(root);
  g_object_unref(builder);
  return content;
}

/** handler for /providers api request */
static void
_service_providers_request_handler(SoupServer *server, SoupMessage *msg, const char *path,
				   GHashTable *query, SoupClientContext *client, gpointer user_data)
{
  cio_service_t *service;
  gsize length;
  char *content;

  service = (cio_service_t *)user_data;

  if (msg->method != SOUP_METHOD_GET)
  {
    soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
    return;
  }

  if (g_strcmp0(path, "/providers") != 0)
  {
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  /* get offset and limit from query */
  gchar *value;
  gsize offset = 0;
  gsize limit = 100;
  if (query)
  {
    value = g_hash_table_lookup(query, "offset");
    if (value)
      offset = g_ascii_strtoll(value, NULL, 10);

    value = g_hash_table_lookup(query, "limit");
    if (value)
      limit = g_ascii_strtoll(value, NULL, 10);

  }

  content = _service_providers_to_json(service, &length, offset, limit);
  soup_message_set_response(msg,
			    "application/json; charset=utf-8",
			    SOUP_MEMORY_TAKE,
			    content,
			    length);

  soup_message_set_status(msg, 200);
}

/** handler for /backlog api request */
static void
_service_backlog_request_handler(SoupServer *server, SoupMessage *msg, const char *path,
				 GHashTable *query, SoupClientContext *client, gpointer user_data)
{
  cio_service_t *service;
  gsize length;
  char *content;

  service = (cio_service_t *)user_data;

  if (msg->method != SOUP_METHOD_GET)
  {
    soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
    return;
  }

  if (g_strcmp0(path, "/backlog") != 0)
  {
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  content = _service_backlog_to_json(service, &length);
  soup_message_set_response(msg,
			    "application/json; charset=utf-8",
			    SOUP_MEMORY_TAKE,
			    content,
			    length);

  soup_message_set_status(msg, 200);
}

/** handler for /cache api request */
static void
_service_cache_request_handler(SoupServer *server, SoupMessage *msg, const char *path,
			       GHashTable *query, SoupClientContext *client, gpointer user_data)
{
  cio_service_t *service;
  gchar *resource;
  gchar *data;
  size_t size;

  service = (cio_service_t *)user_data;

  if (msg->method != SOUP_METHOD_GET)
  {
    soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
    return;
  }

  if (g_strcmp0(path, "/cache") != 0)
  {
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  if (!query)
  {
    soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
    return;
  }

  /* Get the resource url from query and unescape it */
  resource = g_hash_table_lookup(query, "resource");
  if (!resource)
  {
    soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
    return;
  }

  resource = g_uri_unescape_string(resource, NULL);
  if (!resource)
  {
    soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
    return;
  }

  g_log(DOMAIN, G_LOG_LEVEL_INFO, "Retreiving '%s' through service cache.", resource);

  /* lookup resource in cache */
  if (cio_blobcache_get(service->blobcache, g_str_hash(resource), (void **)&data, &size) == 0)
  {
    char *image;
    cio_blobcache_resource_header_t *hdr;

    hdr = (cio_blobcache_resource_header_t *)data;

    /* resource not available return not found... */
    if (hdr->mime[0] == 0 && hdr->size == 0)
    {
      soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
      return;
    }

    image = g_malloc(hdr->size);
    memcpy(image, data + sizeof(cio_blobcache_resource_header_t), hdr->size);

    soup_message_set_response(msg, hdr->mime,
			      SOUP_MEMORY_TAKE,
			      image, size);

    soup_message_set_status(msg, 200);

    g_free(data);
    g_free(resource);
    return;
  }

  /* TODO: resource not found in cache, lets download and add it to cache */
  {
    guint status;
    const gchar *mime;
    GHashTable *params;
    SoupSession *session;
    SoupMessage *gmsg;

    session = soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE,
					    SOUP_SESSION_FEATURE(service->cache),
					    NULL);

    gmsg = soup_message_new("GET", resource);
    if (!gmsg)
    {
      g_log(DOMAIN, G_LOG_LEVEL_WARNING, "Failed to fetch resource uri '%s' into cache", resource);
      g_object_unref(session);
      return;
    }


    soup_message_headers_replace(gmsg->request_headers, "Accept-Charset", "utf-8");
    status = soup_session_send_message(session, gmsg);
    mime = soup_message_headers_get_content_type(gmsg->response_headers, &params);
    soup_cache_flush(service->cache);

    if (status != 200)
    {
      /* resource not availble create a empty cache item to prevent
	 subsequential fetches for a short time, 30 minutes... */
      cio_blobcache_resource_header_t hdr;
      memset(&hdr, 0, sizeof(hdr));

      cio_blobcache_store(service->blobcache, 60 * 30, g_str_hash(resource), &hdr, sizeof(hdr));
      soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
      return;
    }

    /* create blob and store in blobcache */
    cio_blobcache_resource_header_t *hdr;
    size_t blob_size = gmsg->response_body->length + sizeof(cio_blobcache_resource_header_t);
    uint8_t *blob = g_malloc(blob_size);
    memset(blob, 0, sizeof(blob_size));

    hdr = (cio_blobcache_resource_header_t *)blob;
    hdr->size = gmsg->response_body->length;
    snprintf(hdr->mime, sizeof(hdr->mime), "%s", mime);

    memcpy(blob + sizeof(cio_blobcache_resource_header_t),
	   gmsg->response_body->data, gmsg->response_body->length);

    /* FIXME: get cache liftime and use it probably when puttin
       resource in internal blob cache. */
    cio_blobcache_store(service->blobcache, 0, g_str_hash(resource), blob, blob_size);

    /* send content to client */
    char *data;
    data = malloc(hdr->size);
    memcpy(data, gmsg->response_body->data, gmsg->response_body->length);
    g_object_unref(session);
    g_object_unref(gmsg);

    soup_message_set_response(msg, hdr->mime,
			    SOUP_MEMORY_TAKE,
			    data, hdr->size);
    soup_message_set_status(msg, 200);

    g_free(blob);
  }

}


static void
_service_initialize_handler(cio_service_t *self)
{
  GList *list, *item;
  cio_provider_descriptor_t *provider;
  gchar path[512];

  /* add handler for backlog */
  soup_server_add_handler(self->priv->server, "/backlog",
			  _service_backlog_request_handler,
			  self, NULL);

  /* add handler for configuration */
  soup_server_add_handler(self->priv->server, "/settings",
			  cio_settings_request_handler,
			  self->settings, NULL);

  /* add handler for search */
  soup_server_add_handler(self->priv->server, "/search",
			  cio_search_request_handler,
			  self, NULL);


  /* add handler for providers */
  soup_server_add_handler(self->priv->server, "/providers",
			  _service_providers_request_handler,
			  self, NULL);

  /* add handlers for each provider plugin */
  list = item = g_hash_table_get_values(self->providers);
  while(item)
  {
    provider = item->data;
    g_assert(provider != NULL);

    /* register plugin web api handlers */
    g_snprintf(path, sizeof(path), "/providers/%s", provider->id);
    soup_server_add_handler(self->priv->server, path,
			    cio_provider_request_handler,
			    self, NULL);

    item = g_list_next(item);
  }
  g_list_free(list);


  /* add handler for cache */
  soup_server_add_handler(self->priv->server, "/cache",
			  _service_cache_request_handler,
			  self, NULL);
}

/** handler for auth domain */
static char *
_service_auth_domain_handler(SoupAuthDomain *domain, SoupMessage *msg,
			     const char *username, gpointer user_data)
{
  gchar *digest;
  cio_service_t *service;

  service = (cio_service_t *)user_data;
  digest = cio_settings_get_string_value(service->settings,
					 "service", "auth_digest", NULL);
  if (digest == NULL)
    return NULL;

  return digest;
}

cio_service_t *
cio_service_new()
{
  cio_service_t *service;

  service = g_malloc(sizeof(cio_service_t));
  memset(service,0,sizeof(cio_service_t));

  service->priv = g_malloc(sizeof(cio_service_priv_t));
  memset(service->priv, 0, sizeof(cio_service_priv_t));

  g_log_set_default_handler(_service_log_handler, service);

  service->providers = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)cio_provider_destroy);
  service->priv->backlog = g_queue_new();

  /* initialize soup cache for plugin http requests */
  service->cache = soup_cache_new(CASTIO_INSTALL_PREFIX"/var/cache/castio/http",
				  SOUP_CACHE_SINGLE_USER);
  soup_cache_set_max_size(service->cache, 200L*1024L*1024L);
  soup_cache_load(service->cache);

  /* initialize blobcache */
  service->blobcache = cio_blobcache_new();

  return service;
}

void
cio_service_destroy(struct cio_service_t *self)
{
  g_object_unref(self->priv->server);
  g_object_unref(self->priv->domain);

  if (self->settings)
    cio_settings_destroy(self->settings);

  if (self->search)
    cio_search_destroy(self->search);

  if (self->blobcache)
    cio_blobcache_destroy(self->blobcache);

  if (self->cache)
    g_object_unref(self->cache);

  while(!g_queue_is_empty(self->priv->backlog))
    json_node_free(g_queue_pop_head(self->priv->backlog));
  g_queue_free(self->priv->backlog);

  g_hash_table_destroy(self->providers);

  g_free(self->priv);
  g_free(self);
}

static void
_service_request_read_handler(SoupServer *server,
			      SoupMessage *message,
			      SoupClientContext *client,
			      gpointer opaque)
{
  g_log(DOMAIN, G_LOG_LEVEL_DEBUG, "%s %s %s",
	soup_client_context_get_host(client), message->method,
	soup_uri_get_path(soup_message_get_uri(message)));
}

gboolean
cio_service_initialize(struct cio_service_t *self,
		       int argc, char **argv)
{
  GOptionContext *option;
  GError *err;

  err = NULL;

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"Initialize of CAST.IO service.");

  /* Parse and handle arguments */
  option = g_option_context_new("- CAST.IO service");
  g_option_context_add_main_entries(option, entries, NULL);
  if (!g_option_context_parse(option, &argc, &argv, &err))
  {
    g_log(DOMAIN, G_LOG_LEVEL_ERROR,
	"Failed to parse option: %s", err->message);
    return FALSE;
  }
  g_option_context_free(option);

  self->priv->config_file = g_option_config_file;

  /* initialize and load configuration from file */
  self->settings = cio_settings_new(self->priv->config_file);

  _service_configuration_defaults(self);

  /* initialize internal and plugin providers */
  _service_initialize_providers(self);

  /* intialize search */
  self->search = cio_search_new();

  /* initialize soup server */
  self->priv->domain = soup_auth_domain_digest_new(SOUP_AUTH_DOMAIN_REALM, AUTH_REALM, NULL);
  soup_auth_domain_digest_set_auth_callback(self->priv->domain, _service_auth_domain_handler, self, NULL);
  soup_auth_domain_add_path(self->priv->domain, "/");

  self->priv->server = g_object_new(SOUP_TYPE_SERVER, NULL);
  if (self->priv->server == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_CRITICAL,
	  "Failed to instantiate HTTP server.");
    return FALSE;
  }

  soup_server_add_auth_domain(self->priv->server, self->priv->domain);
  g_signal_connect(self->priv->server, "request-read",
		   G_CALLBACK(_service_request_read_handler), self);

  /* initialize service handler */
  _service_initialize_handler(self);

  return TRUE;
}

void
cio_service_main(cio_service_t *self)
{
  guint port;
  GError *err;

  err = NULL;

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"Web service ready for handling clients.");

  /* start run the soup server */
  port = cio_settings_get_int_value(self->settings, "service", "port", &err);
  if (err)
  {
    g_log(DOMAIN, G_LOG_LEVEL_CRITICAL,
	  "Failed to get service port from settings: %s", err->message);
    return;
  }

  soup_server_listen_all(self->priv->server, port, 0, &err);
  if (err)
  {
    g_log(DOMAIN, G_LOG_LEVEL_CRITICAL,
	  "Failed to listen for connections: %s", err->message);
    return;
  }

  g_log(DOMAIN, G_LOG_LEVEL_INFO, "Using port %d for connections.", port);

  /* start running main loop */
  self->priv->loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(self->priv->loop);
  g_main_loop_unref(self->priv->loop);

  /* Store configuration from memory to file. */
  if (!cio_settings_save(self->settings, &err))
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to save settings: %s", err->message);

  /* dump soup cache index */
  soup_cache_dump(self->cache);
}

void
cio_service_quit(cio_service_t *self)
{
  g_log(DOMAIN, G_LOG_LEVEL_DEBUG,
	"Existing main loop.");
  g_main_loop_quit(self->priv->loop);
}
