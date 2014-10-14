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
#include <errno.h>

#include <glib.h>
#include <libsoup/soup.h>

#include "config.h"
#include "service.h"
#include "search.h"
#include "settings.h"
#include "provider.h"

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
} cio_service_priv_t;

/** setup default values for missing keys in configuration */
static void
_service_configuration_defaults(cio_service_t *self)
{
  GError *err;
  char *digest;
  JsonNode *value;

  err = NULL;

  /* intialize listen port */
  value = json_node_alloc();
  value = json_node_init_int(value, 1457);
  cio_settings_create_value(self->settings, "service", "port",
			    "Service port",
			    "Port were the service listens for web api requests."
			    " Requires a restart of the service.",
			    value, &err);

  /* intialize plugin directory */
  g_clear_error(&err),
  value = json_node_alloc();
  value = json_node_init_string(value, CASTIO_DATA_DIR"/plugins");
  cio_settings_create_value(self->settings, "service", "plugin_dir",
			    "Plugin directory",
			    "Specifies were the service searchs for plugins to be loaded."
			    " Requires a restart of the service.",
			    value, &err);

  /* intialize digest */
  g_clear_error(&err),
  digest = soup_auth_domain_digest_encode_password("admin", AUTH_REALM, "password");
  value = json_node_alloc();
  value = json_node_init_string(value, digest);
  cio_settings_create_value(self->settings, "service", "auth_digest",
			    "Authenticatoin digest",
			    "The encoded authentication digest used for accessing the web api.",
			    value, &err);

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

  /* instantiate internal providers */
  provider = cio_provider_instance(self, CIO_PROVIDER_MOVIE_LIBRARY, NULL);
  if (provider)
    g_hash_table_insert(self->providers, provider->id, provider);
  else
    g_warning("Failed to instantiate movie library provider.");

  /* load and instantiate available plugin providers */
  plugindir = cio_settings_get_string_value(self->settings, "service", "plugin_dir", &err);
  if (err)
  {
    g_warning("No plugin directory specified in configuration.");
    return;
  }

  dir = g_dir_open(plugindir, 0, &err);
  if (err)
  {
    g_warning("Failed to load plugins: %s", err->message);
    g_free(plugindir);
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

  g_free(plugindir);
}


/** create json array of available providers */
static gchar *
_service_providers_to_json(cio_service_t *self, gsize *length)
{
  cio_provider_descriptor_t *provider;
  JsonGenerator *gen;
  JsonBuilder *builder;
  JsonNode *root;
  GList *item;
  gchar *content;

  item = g_hash_table_get_values(self->providers);

  builder = json_builder_new();
  builder = json_builder_begin_array(builder);
  while (item)
  {
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
    builder = json_builder_add_string_value(builder, provider->version);
    builder = json_builder_set_member_name(builder, "copyright");
    builder = json_builder_add_string_value(builder, provider->copyright);
    builder = json_builder_set_member_name(builder, "homepage");
    builder= json_builder_add_string_value(builder, provider->homepage);
    builder = json_builder_set_member_name(builder, "icon");
    builder= json_builder_add_string_value(builder, provider->icon);

    builder = json_builder_end_object(builder);
    item = g_list_next(item);
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

  content = _service_providers_to_json(service, &length);
  soup_message_set_response(msg,
			    "application/json",
			    SOUP_MEMORY_TAKE,
			    content,
			    length);

  soup_message_set_status(msg, 200);
}


static void
_service_initialize_handler(cio_service_t *self)
{
  GList *item;
  cio_provider_descriptor_t *provider;
  gchar path[512];


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
  item = g_hash_table_get_values(self->providers);
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
}

/** handler for auth domain */
static char *
_service_auth_domain_handler(SoupAuthDomain *domain, SoupMessage *msg,
			     const char *username, gpointer user_data)
{
  GError *err;
  gchar *digest;
  cio_service_t *service;

  service = (cio_service_t *)user_data;
  err = NULL;

  digest = cio_settings_get_string_value(service->settings, "service", "auth_digest", &err);
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

  service->providers = g_hash_table_new(g_str_hash, g_str_equal);

  return service;
}

void
cio_service_destroy(struct cio_service_t *self)
{
  if (self->settings)
    cio_settings_destroy(self->settings);

  if (self->search)
    cio_search_destroy(self->search);

  g_free(self->priv);
  g_free(self);
}


gboolean
cio_service_initialize(struct cio_service_t *self,
		       int argc, char **argv)
{
  guint port;
  SoupAddress *addr;
  GOptionContext *option;
  GError *err;

  err = NULL;

  /* Parse and handle arguments */
  option = g_option_context_new("- cast.io service");
  g_option_context_add_main_entries(option, entries, NULL);
  if (!g_option_context_parse(option, &argc, &argv, &err))
  {
    g_print("Parse options failed: %s\n", err->message);
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

  port = cio_settings_get_int_value(self->settings, "service", "port", &err);
  if (err)
  {
    g_print("ERROR: Failed to get service port from settings: %s\n", err->message);
    return FALSE;
  }

  self->priv->server = soup_server_new(SOUP_SERVER_PORT, port, NULL);
  if (self->priv->server == NULL)
  {
    g_critical("Failed start server on port %d: %s",port, strerror(errno));
    return FALSE;
  }

  soup_server_add_auth_domain(self->priv->server, self->priv->domain);

  /* initialize service handler */
  _service_initialize_handler(self);

  return TRUE;
}

void
cio_service_main(cio_service_t *self)
{
  GError *err;

  err = NULL;

  /* start run the soup server */
  soup_server_run_async(self->priv->server);

  /* star running main loop */
  self->priv->loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(self->priv->loop);

  /* Store configuration from memory to file. */
  if (!cio_settings_save(self->settings, &err))
    g_warning("Failed to save settings: %s", err->message);

}

void
cio_service_quit(cio_service_t *self)
{
  g_main_loop_quit(self->priv->loop);
}
