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
#include "settings.h"

#define DOMAIN "settings"

typedef struct cio_settings_t
{
  char *filename;
  JsonNode *root;
} cio_settings_t;

static GQuark _quark;

static gboolean
_settings_validate(cio_settings_t *self, GError **err)
{
  GList *sections, *sit;
  GList *members, *mit;
  JsonObject *object;
  JsonObject *settings;
  JsonObject *setting;
  JsonNode *value;

  g_assert(self->root != NULL);

  /* verify that root is an object */
  object = json_node_get_object(self->root);
  if (object == NULL)
  {
    g_set_error(err, _quark, 3, "Settings root is not an object.");
    return FALSE;
  }

  /* if object is empty consider it as valid */
  sit = sections = json_object_get_members(object);
  if (sections == NULL)
    return TRUE;

  /* validate each section */
  do
  {
    settings = json_object_get_object_member(object, sit->data);
    g_assert(settings != NULL);

    /* verify that section has settings defined */
    mit = members = json_object_get_members(settings);
    if (!members)
    {
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "section '%s' is empty, removing it.", (gchar *)sit->data);
      json_object_remove_member(object, sit->data);
      continue;
    }

    /* verify that each setting has members; name, description, value */
    do {

      setting = json_object_get_object_member(settings, mit->data);

      /* verify name exists and that it is a string */
      if (!json_object_has_member(setting, "name"))
      {
	g_set_error(err, _quark, 3, "Setting '%s' in section '%s' doesn't have a \"name\" member.",
		    (gchar *)sit->data, (gchar *)mit->data);
	goto bail_out;
      }

      value = json_object_get_member(setting, "name");
      if (!g_type_is_a(json_node_get_value_type(value), G_TYPE_STRING))
      {
	g_set_error(err, _quark, 3, "Setting '%s' value in section '%s' is not a string.",
		    (gchar *)sit->data, (gchar *)mit->data);
	goto bail_out;
      }

      /* verify description exists and that it is a string */
      if (!json_object_has_member(setting, "description"))
      {
	g_set_error(err, _quark, 3, "Setting '%s' in section '%s' doesn't have a \"description\" member.",
		    (gchar *)sit->data, (gchar *)mit->data);
	goto bail_out;
      }

      value = json_object_get_member(setting, "description");
      if (!g_type_is_a(json_node_get_value_type(value), G_TYPE_STRING))
      {
	g_set_error(err, _quark, 3, "Setting '%s' value in section '%s' is not a string.",
		    (gchar *)sit->data, (gchar *)mit->data);
	goto bail_out;
      }

      /* verify value exists and that the node holds a value */
      if (!json_object_has_member(setting, "value"))
      {
	g_set_error(err, _quark, 3, "Setting '%s' in section '%s' doesn't have a \"value\" member.",
		    (gchar *)sit->data, (gchar *)mit->data);
	goto bail_out;
      }
      value = json_object_get_member(setting, "value");
      if (!JSON_NODE_HOLDS_VALUE(value))
      {
	g_set_error(err, _quark, 3, "Setting '%s' value in section '%s' is not a JSON value.",
		    (gchar *)sit->data, (gchar *)mit->data);
	goto bail_out;
      }

    } while ((mit = g_list_next(mit)) != NULL);

    g_list_free(members);
    members = NULL;

  } while ((sit = g_list_next(sit)) != NULL);

  g_list_free(sections);
  return TRUE;

bail_out:
  g_list_free(members);
  g_list_free(sections);
  return FALSE;
}

cio_settings_t *
cio_settings_new(const char *filename)
{
  GError *err;
  cio_settings_t *settings;

  err = NULL;

  _quark = g_quark_from_static_string ("settings");

  settings = g_malloc(sizeof(cio_settings_t));
  memset(settings, 0, sizeof(cio_settings_t));

  settings->filename = g_strdup(filename);

  if (!cio_settings_load(settings, &err))
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to load settings: %s", err->message);
    settings->root = json_node_alloc();
    settings->root = json_node_init_object(settings->root, json_object_new());
  }

  return settings;
}

void
cio_settings_destroy(cio_settings_t *self)
{
  GError *err;
  err = NULL;

  cio_settings_save(self, &err);

  g_free(self->filename);
  json_node_free(self->root);
  g_free(self);
}

gboolean
cio_settings_load(cio_settings_t *self, GError **err)
{
  JsonParser *parser;

  parser = json_parser_new();
  if(!json_parser_load_from_file(parser, self->filename, err))
  {
    g_object_unref(parser);
    return FALSE;
  }

  self->root = json_parser_get_root(parser);
  self->root = json_node_copy(self->root);

  g_object_unref(parser);

  if (!_settings_validate(self, err))
  {
    json_node_free(self->root);
    self->root = NULL;
    return FALSE;
  }
  return TRUE;
}

gboolean
cio_settings_save(cio_settings_t *self, GError **err)
{
  JsonGenerator *gen;

  gen = json_generator_new();
  json_generator_set_pretty(gen, TRUE);
  json_generator_set_root(gen, self->root);

  json_generator_to_file(gen, self->filename, err);
  g_object_unref(gen);
  if (*err)
    return FALSE;

  return TRUE;
}


gboolean
cio_settings_has_section(cio_settings_t *self, const char *section)
{
  JsonObject *object;
  object = json_node_get_object(self->root);
  g_assert(object != NULL);

  return json_object_has_member(object, section);
}

gboolean
cio_settings_update_section(struct cio_settings_t *self,
			    const char *section,
			    JsonNode *node)
{
  GList *members;
  JsonObject *object;
  JsonObject *src;
  JsonObject *dest;
  JsonObject *setting;
  JsonNode *temp, *sval, *dval;

  /* validate section */
  object = json_node_get_object(self->root);
  g_assert(object != NULL);

  if (!json_object_has_member(object, section))
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to update section '%s', section does not exist.", section);
    return FALSE;
  }

  dest = json_object_get_object_member(object, section);

  /*
   * validate source settings object
   */
  /* FIXME: Do we need to do this ? */
  if (!JSON_NODE_HOLDS_OBJECT(node))
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to update section '%s', json node is not an object", section);
    return FALSE;
  }

  src = json_node_get_object(node);

  /* Verify that source settings object  has members */
  members = json_object_get_members(src);
  if (members == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to update section '%s', empty source object.", section);
    return FALSE;
  }

  /*
   * Verify that each setting in source is an object that holds a
   * member named value and is the same data type as destination
   * value.
   */
  do {
    /* verify source setting is an settings object */
    temp = json_object_get_member(src, members->data);
    if (!JSON_NODE_HOLDS_OBJECT(temp))
    {
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "Failed to update section '%s' setting, member '%s' is not an setting object.",
	    section, (gchar *)members->data);
      return FALSE;
    }

    setting = json_node_get_object(temp);

    /* verify that setting has a value */
    if (!json_object_has_member(setting, "value"))
    {
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "Failed to update section '%s' setting, '%s' does not have a \"value\" member.",
	    section, (gchar *)members->data);
      return FALSE;
    }

    /* verify that destination has the same setting */
    if (!json_object_has_member(dest, members->data))
    {
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "Failed to update section '%s' setting, trying update of non existing setting '%s'.",
	    section, (gchar *)members->data);
      return FALSE;
    }

    /* verify that source value is of same data type as dest */
    sval = json_object_get_member(setting, "value");
    dval = json_object_get_member(json_object_get_object_member(dest, members->data), "value");

    if (json_node_get_value_type(sval) != json_node_get_value_type(dval))
    {
      g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	    "Failed to update section '%s' setting, value type of '%s' is different.",
	    section, (gchar *)members->data);
      return FALSE;
    }

  } while((members = g_list_next(members)) != NULL);


  /* verify that destination settings object have members */
  members = json_object_get_members(dest);
  if (members == NULL)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to update section, section '%s' is empty", section);
    return FALSE;
  }

  /*
   * update settings from source object to dest
   */
  do
  {
    /* skip setting if not found in source settings object */
    if (!json_object_has_member(src, members->data))
      continue;

    /* update dest value with source value */
    json_object_set_member(json_object_get_object_member(dest, members->data),
			   "value", json_node_copy(sval));
    g_log(DOMAIN, G_LOG_LEVEL_DEBUG,
	  "Value of '%s' in section '%s' updated.",
	  (gchar *)members->data, section);
  } while((members = g_list_next(members)) != NULL);


  return TRUE;
}


gboolean
cio_settings_has_value(cio_settings_t *self,
		       const char *section,
		       const char *id)
{
  JsonObject *object, *settings;

  object = json_node_get_object(self->root);
  g_assert(object != NULL);

  settings = json_object_get_object_member(object, section);
  if (settings == NULL)
    return FALSE;

  if (!json_object_has_member(settings, id))
    return FALSE;

  return TRUE;
}

JsonNode *
cio_settings_get_value(cio_settings_t *self,
		       const gchar *section,
		       const gchar *id,
		       GError **err)
{
  JsonObject *object;
  JsonObject *settings;
  JsonObject *setting;

  object = json_node_get_object(self->root);
  g_assert(object != NULL);

  settings = json_object_get_object_member(object, section);
  if (settings == NULL)
  {
    g_set_error(err, _quark, 1, "Section '%s' not found.", section);
    return NULL;
  }

  setting = json_object_get_object_member(settings, id);
  if (setting == NULL)
  {
    g_set_error(err, _quark, 2, "Setting '%s' in section '%s' not found.", id, section);
    return NULL;
  }

  return json_object_get_member(setting, "value");
}

gboolean
cio_settings_create_value(cio_settings_t *self,
			  const char *section,
			  const char *id,
			  const char *name,
			  const char *description,
			  JsonNode *value,
			  GError **err)
{
  JsonObject *object;
  JsonObject *settings;
  JsonObject *setting;

  g_assert(section != NULL);
  g_assert(id != NULL);
  g_assert(name != NULL);
  g_assert(description != NULL);

  object = json_node_get_object(self->root);
  g_assert(object != NULL);

  /* create section if not exists */
  if (!json_object_has_member(object, section))
  {
    g_log(DOMAIN, G_LOG_LEVEL_INFO,
	  "Creating new section '%s'", section);
    settings = json_object_new();
    json_object_set_object_member(object, section, settings);
  }
  else
    settings = json_object_get_object_member(object, section);

  /* check if value already exists */
  if (json_object_has_member(settings, id))
  {
    g_set_error(err, _quark, 5, "Setting '%s' already exists in section '%s'", id, section);
    return FALSE;
  }

  /* create new setting */
  setting = json_object_new();
  json_object_set_object_member(settings, id, setting);
  json_object_set_string_member(setting, "name", name);
  json_object_set_string_member(setting, "description", description);
  json_object_set_member(setting, "value", value);

  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"Created setting '%s' in section '%s'.", id, section);

  return TRUE;
}

char *
cio_settings_get_string_value(cio_settings_t *self,
			      const char *section,
			      const char *id,
			      GError **err)
{
  JsonNode *node;
  node = cio_settings_get_value(self, section, id, err);
  if (node == NULL)
    return NULL;

  return json_node_dup_string(node);
}

int
cio_settings_get_int_value(cio_settings_t *self,
			   const char *section,
			   const char *id,
			   GError **err)
{
  JsonNode *node;
  node = cio_settings_get_value(self, section, id, err);
  if (node == NULL)
    return 0;

  return json_node_get_int(node);
}

gboolean
cio_settings_get_boolean_value(struct cio_settings_t *self,
			       const char *section,
			       const char *id,
			       GError **err)
{
  JsonNode *node;
  node = cio_settings_get_value(self, section, id, err);
  if (node == NULL)
    return FALSE;

  return json_node_get_boolean(node);
}


void cio_settings_request_handler(SoupServer *server, SoupMessage *msg,
				  const char *path, GHashTable *query,
				  SoupClientContext *client, gpointer user_data)
{
  GError *err;
  gsize length;
  const gchar *mime_type;
  gchar *content;
  gchar **components;
  JsonParser *parser;
  JsonGenerator *gen;
  JsonObject *object;
  JsonNode *node;
  cio_settings_t *settings;

  settings = (cio_settings_t *)user_data;

  err = NULL;

  /* we will only handle /settings/<section> paths */
  components = g_strsplit(path, "/", -1);
  if (g_strv_length(components) != 3)
  {
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  /* return if section is not found */
  if (!cio_settings_has_section(settings, components[2]))
  {
    soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
    return;
  }

  /* verify valid request method */
  if (msg->method != SOUP_METHOD_GET && msg->method != SOUP_METHOD_PUT)
  {
    soup_message_set_status(msg, SOUP_STATUS_METHOD_NOT_ALLOWED);
    return;
  }

  /*
   * Handle GET request
   */
  if (msg->method == SOUP_METHOD_GET)
  {
    /* get section node */
    object = json_node_get_object(settings->root);
    node = json_object_get_member(object, components[2]);
    g_assert(node != NULL);

    /* generate json data out of settings */
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

  }

  /*
   * Handle PUT request
   */
  else if (msg->method == SOUP_METHOD_PUT)
  {
    /* only support requests with content-type application/json */
    mime_type = soup_message_headers_get_content_type(msg->request_headers, NULL);
    if (strncmp(mime_type, "application/json", strlen("application/json")) != 0)
    {
      soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
      return;
    }

    /* parse json body from request */
    parser = json_parser_new();
    if (!json_parser_load_from_data(parser,
				    msg->request_body->data,
				    msg->request_body->length, &err))
    {
      g_object_unref(parser);
      soup_message_set_status(msg, SOUP_STATUS_MALFORMED);
      return;
    }

    /* update configuration */
    if (!cio_settings_update_section(settings, components[2], json_parser_get_root(parser)))
    {
      g_object_unref(parser);
      soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
      return;
    }

    /* cleanup */
    g_object_unref(parser);
  }

  /* return 200 */
  soup_message_set_status(msg, SOUP_STATUS_OK);
}
