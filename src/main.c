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
#include <glib.h>
#include <stdlib.h>

#include "service.h"

void
_log_fn(const gchar *log_domain,
	GLogLevelFlags log_level,
	const gchar *message,
	gpointer user_data)
{
  gchar *level;
  FILE *o;
  if (log_level & (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL
		   | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_INFO))
    o = stderr;
  else
    o = stdout;

  level = "Undefined";
  if (log_level & G_LOG_LEVEL_ERROR)
    level = "Error";
  else if (log_level & G_LOG_LEVEL_CRITICAL)
    level = "Critical";
  else if (log_level & G_LOG_LEVEL_WARNING)
    level = "Warning";
  if (log_level & G_LOG_LEVEL_MESSAGE)
    level = "Message";
  else if (log_level & G_LOG_LEVEL_INFO)
    level = "Info";
  else if (log_level &  G_LOG_LEVEL_DEBUG)
    level = "Debug";

  fprintf(o, "%s [%s] %s\n", level, log_domain, message);

  if (log_level & (G_LOG_FLAG_FATAL))
    abort();
}

cio_service_t *g_service;

void sigint_handler(int sig)
{
  cio_service_quit(g_service);
}

int main(int argc, char **argv)
{
  g_log_set_default_handler(_log_fn, NULL);

  g_service = cio_service_new();

  if (!cio_service_initialize(g_service, argc, argv))
    return 1;

  signal(SIGINT, sigint_handler);
  cio_service_main(g_service);

  cio_service_destroy(g_service);

  return 0;
}
