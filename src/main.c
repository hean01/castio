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

#include <glib.h>
#include <stdlib.h>

#include "service.h"

cio_service_t *g_service;

void sigint_handler(int sig)
{
  cio_service_quit(g_service);
}

int main(int argc, char **argv)
{

  g_service = cio_service_new();

  if (!cio_service_initialize(g_service, argc, argv))
    return 1;

  signal(SIGINT, sigint_handler);
  cio_service_main(g_service);

  cio_service_destroy(g_service);

  return 0;
}
