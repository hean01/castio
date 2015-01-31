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

#include <stdio.h>
#include <libsoup/soup.h>
#include <libxml/HTMLparser.h>
#include <string.h>

#include "service.h"
#include "provider.h"

#include "js/js.h"

#define DOMAIN "provider"

static gchar *
_unescape_buffer(gchar *buffer)
{
  int res;
  gchar name[128];
  gchar *in, *out, *pe;
  const htmlEntityDesc *desc;

  in = out = buffer;

  while(*in)
  {
    if (*in != '&')
    {
      *out = *in;
      in++;
      out++;
      continue;
    }

    in++;

    pe = in;
    while (*pe && *pe != ';') pe++;

    desc = NULL;
    if (*in != '#')
    {
      g_snprintf(name, 1 + pe - in, "%s", in);
      /* TODO: do our own implementation so we can drop libxml2
	 dependency.*/
      desc = htmlEntityLookup((const xmlChar *)name);
    }
    else
    {
      res = g_ascii_strtoll(in + 1, NULL, 10);
      desc = htmlEntityValueLookup(res);
    }

    /* if we couldn't lookup the entity to a unicode value
       just copy over the string. */
    if (desc == NULL)
    {
      *out = '&';
      out++;
      continue;
    }

    res = g_unichar_to_utf8(desc->value, out);
    out += res;
    in = pe + 1;
  }

  *out = '\0';
  return buffer;
}

static void
_js_http_unescape_html(js_State *state)
{
  gchar *buffer;
  buffer = (gchar *)js_tostring(state, 1);
  js_pushstring(state, _unescape_buffer(buffer));
}

static void
_js_http_get(js_State *state)
{
  GError *err;
  const gchar *content;
  gchar *temp, *charset;
  gsize len;
  guint status;
  js_provider_t *js;
  const char *uri;
  SoupSession *session;
  SoupMessage *msg;
  SoupMessageHeadersIter iter;
  const gchar *name, *value;
  GList *keys, *kit;
  JsonNode *headers;
  JsonObject *object;
  GHashTable *params;
  const gchar *ctype;

  err = NULL;
  temp = NULL;
  headers = NULL;
  params = NULL;
  js = js_touserdata(state, 0, "instance");

  uri = js_tostring(state, 1);
  if (!js_isundefined(state, 2))
    headers = js_util_tojsonnode(state, 2);

  session = soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE,
					  SOUP_SESSION_FEATURE(js->provider->service->cache),
					  NULL);

  g_log(DOMAIN, G_LOG_LEVEL_DEBUG,
	"[%s.http.get] resource '%s'", js->provider->id, uri);

  msg = soup_message_new("GET", uri);
  if (msg == NULL)
  {
    js_error(state, "Invalid uri '%s'", uri);
    return;
  }

  /* setup headers for request */
  soup_message_headers_replace(msg->request_headers, "Accept-Charset", "utf-8");
  if (headers)
  {
    object = json_node_get_object(headers);
    keys = kit = json_object_get_members(object);
    while (kit)
    {
      value = json_object_get_string_member(object, keys->data);
      if (value)
        soup_message_headers_replace(msg->request_headers, (char *)keys->data, value);
      kit = g_list_next(kit);
    }
    g_list_free(keys);
    json_node_free(headers);
  }


  status = soup_session_send_message(session, msg);

  params = NULL;
  ctype = soup_message_headers_get_content_type(msg->response_headers, &params);

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"[%s.http.get] GET '%s' %d, %s bytes '%s'",
	js->provider->id, uri, status, ctype,
	soup_message_headers_get_one(msg->response_headers, "Content-Type"));

  /* convertion of body encoding to utf-8 */
  charset = g_hash_table_lookup(params, "charset");
  content = msg->response_body->data ? msg->response_body->data : "";
  if (charset && msg->response_body->data)
  {
    g_log(DOMAIN, G_LOG_LEVEL_DEBUG,
	  "[%s.http.get] Converting response body from '%s' to 'utf-8'",
	  js->provider->id, charset);

    temp = g_convert(msg->response_body->data, msg->response_body->length,
			"utf-8", charset, NULL, &len, &err);

    if (err)
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "Failed to convert: %s", err->message);
    content = temp;
  };
  g_hash_table_destroy(params);

  /* push result object */
  js_newobject(state);
  {
    js_pushnumber(state, status);
    js_defproperty(state, -2, "status", JS_READONLY);

    /* TODO: add responses headers */
    js_newobject(state);
    soup_message_headers_iter_init(&iter,msg->response_headers);
    while (soup_message_headers_iter_next(&iter, &name, &value))
    {
      js_pushstring(state, value);
      js_defproperty(state, -2, name, JS_READONLY);
    }
    js_defproperty(state, -2, "headers", JS_READONLY);

    js_pushstring(state, content);
    js_defproperty(state, -2, "body", JS_READONLY);
  }

  soup_cache_flush(js->provider->service->cache);
  g_object_unref(session);
  g_object_unref(msg);
  g_free(temp);
}

static void
_js_http_post(js_State *state)
{
  guint status;
  js_provider_t *js;
  const char *uri;
  SoupSession *session;
  SoupMessage *msg;
  SoupMessageHeadersIter iter;
  const gchar *name, *value;
  GList *keys;
  JsonNode *headers;
  JsonObject *object;
  const gchar *content;

  headers = NULL;
  content = NULL;

  js = js_touserdata(state, 0, "instance");

  uri = js_tostring(state, 1);

  if (!js_isundefined(state, 2))
    headers = js_util_tojsonnode(state, 2);

  if (!js_isundefined(state, 3))
    content = js_tostring(state, 3);

  session = soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE,
					  SOUP_SESSION_FEATURE(js->provider->service->cache),
					  NULL);

  g_log(DOMAIN, G_LOG_LEVEL_DEBUG,
	"[%s.http.post] resource '%s'", js->provider->id, uri);

  msg = soup_message_new("POST", uri);
  if (msg == NULL)
  {
    js_error(state, "Invalid uri '%s'", uri);
    return;
  }

  /* set request body if specified */
  if (content)
    soup_message_set_request(msg, "text/plain", SOUP_MEMORY_COPY, content, strlen(content));

  /* setup headers for request */
  if (headers)
  {
    object = json_node_get_object(headers);
    keys = json_object_get_members(object);
    while (keys)
    {
      value = json_object_get_string_member(object, keys->data);
      if (value)
        soup_message_headers_replace(msg->request_headers, (char *)keys->data, value);
      keys = g_list_next(keys);
    }
    json_node_free(headers);
  }

  /* send the request */
  status = soup_session_send_message(session, msg);

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"[%s.http.post] POST '%s' %d, %s bytes '%s'",
	js->provider->id, uri, status,
	soup_message_headers_get_one(msg->response_headers, "Content-Length"),
	soup_message_headers_get_one(msg->response_headers, "Content-Type"));

  /* push result object */
  js_newobject(state);
  {
    js_pushnumber(state, status);
    js_defproperty(state, -2, "status", JS_READONLY);

    /* add response headers */
    js_newobject(state);
    soup_message_headers_iter_init(&iter,msg->response_headers);
    while (soup_message_headers_iter_next(&iter, &name, &value))
    {
      js_pushstring(state, value);
      js_defproperty(state, -2, name, JS_READONLY);
    }
    js_defproperty(state, -2, "headers", JS_READONLY);

    js_pushstring(state, (msg->response_body->data ? msg->response_body->data : ""));
    js_defproperty(state, -2, "body", JS_READONLY);
  }

  soup_cache_flush(js->provider->service->cache);
  g_object_unref(session);
  g_object_unref(msg);
}

void
js_http_init(js_State *state, js_provider_t *instance)
{
  js_newobject(state);
  {
    /* add user data to service */
    js_getproperty(state, 0, "prototype");
    js_newuserdata(state, "instance", instance, NULL);

    js_newcfunction(state, _js_http_get, "get", 2);
    js_defproperty(state, -2, "get", JS_READONLY);

    js_newcfunction(state, _js_http_post, "post", 3);
    js_defproperty(state, -2, "post", JS_READONLY);

    js_newcfunction(state, _js_http_unescape_html, "unescapeHTML", 1);
    js_defproperty(state, -2, "unescapeHTML", JS_READONLY);
  }
}
