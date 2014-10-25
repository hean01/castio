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
#include <json-glib/json-glib.h>

#include "search.h"
#include "service.h"
#include "settings.h"
#include "provider.h"

#define DOMAIN "search"

typedef struct cio_search_t
{
  GHashTable *jobs;
} cio_search_t;

typedef struct _search_job_t
{
  gchar *keywords;
  gint providers;
  JsonObject *result;
} _search_job_t;

static _search_job_t *
_search_job_ctor()
{
  _search_job_t *job;
  job = g_new0(_search_job_t, 1);
  job->result = json_object_new();
  return job;
}

typedef struct _search_provider_job_t
{
  gchar *keywords;
  cio_provider_descriptor_t *provider;
  _search_job_t *sj;
} _search_provider_job_t;


static void
_search_on_item_callback(cio_provider_descriptor_t *provider, JsonNode *item, gpointer user_data)
{
  JsonArray *array;
  _search_job_t *job;

  job = (_search_job_t *)user_data;

  if (item == NULL)
    return;

  /* add provide object if not exists */
  if (!json_object_has_member(job->result, provider->id))
  {
    array = json_array_new();
    json_object_set_array_member(job->result, provider->id, array);
  }

  /* add item to provider result array */
  array = json_object_get_array_member(job->result, provider->id);
  json_array_add_element(array, item);
}

static _search_provider_job_t *
_search_provider_job_ctor(cio_provider_descriptor_t *provider,
			  gchar *keywords, _search_job_t *job)
{
  _search_provider_job_t *j;
  j = g_malloc(sizeof(_search_provider_job_t));
  j->provider = provider;
  j->keywords = g_strdup(keywords);
  j->sj = job;
  return j;
}

static gboolean
_search_provider_job(gpointer user_data)
{
  _search_provider_job_t *job;
  job = user_data;
  job->provider->search(job->provider, job->keywords,
			_search_on_item_callback, job->sj);

  job->sj->providers--;

  g_free(job->keywords);
  g_free(job);
  return FALSE;
}

cio_search_t *
cio_search_new()
{
  cio_search_t *search;

  search = g_malloc(sizeof(cio_search_t));
  memset(search, 0, sizeof(cio_search_t));

  search->jobs = g_hash_table_new_full(g_str_hash, g_str_equal,
				       g_free, NULL);

  return search;
}

void
cio_search_destroy(cio_search_t *self)
{
}

void
cio_search_request_handler(SoupServer *server, SoupMessage *msg,
			   const char *path, GHashTable *query,
			   SoupClientContext *client, gpointer user_data)
{
  gsize vlen;
  gchar *content;
  gsize length;
  gchar **components;
  cio_service_t *service;
  _search_job_t *job;
  GList *iter;
  gchar *keywords;
  gchar *providers;
  gchar *key;
  gchar location[512];
  JsonGenerator *gen;
  JsonNode *node;
  GError *err;
  gboolean enabled;
  cio_provider_descriptor_t *provider;

  job = NULL;
  err = NULL;
  service = (cio_service_t *)user_data;

  /* we will only handle /settings and /settings/<jobid> paths */
  components = g_strsplit(path, "/", -1);
  vlen = g_strv_length(components);
  if (vlen != 2 && vlen != 3)
  {
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  /* this handler only supports GET methods */
  if (msg->method != SOUP_METHOD_GET)
  {
    soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
    return;
  }

  /*
   * perform a search
   */
  if (vlen == 2)
  {
    /* verify that we have a query with keywords to seach for */
    if (query == NULL)
    {
      soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "Search failed, no query specified in the request.");
      return;
    }

    keywords = g_hash_table_lookup(query, "keywords");
    if (keywords == NULL)
    {
      soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "Search failed, no keywords specified in the request.");
      return;
    }


    /* verify that service has registered providers */
    iter = g_hash_table_get_keys(service->providers);
    if (iter == NULL)
    {
      soup_message_set_status(msg, SOUP_STATUS_SERVICE_UNAVAILABLE);
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "Search failed, no providers registered with the service.");
      return;
    }

    providers = g_hash_table_lookup(query, "providers");

    do
    {
      provider = g_hash_table_lookup(service->providers, iter->data);

      if (provider->search == NULL)
	continue;

      /* continue with next if provider is disabled */
      g_clear_error(&err);
      enabled = cio_settings_get_boolean_value(service->settings,
					       provider->id, "enabled", &err);
      if (err == NULL && enabled == FALSE)
      {
	g_log(DOMAIN, G_LOG_LEVEL_INFO,
	      "Provider '%s' is disabled.", provider->id);
	continue;
      }


      /* lookup provider in specified providers list from query */
      if (providers && g_strrstr(providers, provider->id) == NULL)
	continue;

      if (job == NULL)
	job = _search_job_ctor();

      job->providers++;

      /* create a job for each search on main thread */
      g_idle_add(_search_provider_job,
		 _search_provider_job_ctor(provider, keywords, job));

    } while((iter = g_list_next(iter)) != NULL);

    /* verify that we actually have a search job */
    if (job == NULL)
    {
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "Search failed, no matching providers specified in query.");
      soup_message_set_status(msg, SOUP_STATUS_SERVICE_UNAVAILABLE);
      return;
    }

    /* add job to hash table */
    g_snprintf(location, sizeof(location), "%x", g_int64_hash(job));
    g_snprintf(location, sizeof(location), "%x", g_str_hash(location));
    key = g_strdup(location);
    g_hash_table_insert(service->search->jobs, key, job);

    /* build a result uri and add location header to response */
    g_snprintf(location, sizeof(location), "/search/%s", key);
    soup_message_headers_append(msg->response_headers, "Location", location);
    soup_message_set_status(msg, SOUP_STATUS_MOVED_TEMPORARILY);
    return;
  }

  /*
   * get results from a search
   */
  else
  {
    /* get job from hash table */
    job = g_hash_table_lookup(service->search->jobs, components[2]);
    if (job == NULL)
    {
      g_log(DOMAIN, G_LOG_LEVEL_WARNING, "No search result available for '%s'", path);
      soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
      return;
    }

    /* generate json from result as content */
    node = json_node_alloc();
    node = json_node_init_object(node, job->result);

    gen = json_generator_new();
    json_generator_set_pretty(gen, TRUE);
    json_generator_set_root(gen, node);
    content = json_generator_to_data(gen, &length);
    g_object_unref(gen);

    soup_message_set_response(msg,
			      "application/json; charset=utf-8",
			      SOUP_MEMORY_TAKE,
			      content,
			      length);


    /* free result and create new array */
    json_node_free(node);
    job->result = json_object_new();

    /* if job is finished, remove, cleanup and return 200 */
    if (job->providers == 0)
    {
      g_hash_table_remove(service->search->jobs, components[2]);
      g_free(job);
      soup_message_set_status(msg, SOUP_STATUS_OK);
      return;
    }

    soup_message_set_status(msg, SOUP_STATUS_PARTIAL_CONTENT);
    return;
  }

  soup_message_set_status(msg, SOUP_STATUS_NOT_IMPLEMENTED);
}
