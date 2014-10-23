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

    g_log(DOMAIN, G_LOG_LEVEL_DEBUG,
	  "Found unicode value %ul named '%s'", desc->value, desc->name);

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
  guint status;
  js_provider_t *js;
  const char *uri;
  SoupSession *session;
  SoupMessage *msg;
  SoupMessageHeadersIter iter;
  const gchar *name, *value;

  js = js_touserdata(state, 0, "instance");
  uri = js_tostring(state, 1);

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

  soup_message_headers_replace(msg->request_headers, "Accept-Charset", "utf-8");

  status = soup_session_send_message(session, msg);

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"[%s.http.get] GET '%s' %d, %s bytes '%s'",
	js->provider->id, uri, status,
	soup_message_headers_get_one(msg->response_headers, "Content-Length"),
	soup_message_headers_get_one(msg->response_headers, "Content-Type"));

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

    js_pushstring(state, msg->response_body->data);
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
    js_newuserdata(state, "instance", instance);

    js_newcfunction(state, _js_http_get, 1);
    js_defproperty(state, -2, "get", JS_READONLY);

    js_newcfunction(state, _js_http_unescape_html, 1);
    js_defproperty(state, -2, "unescapeHTML", JS_READONLY);
  }
}
