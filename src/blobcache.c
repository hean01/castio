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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "config.h"
#include "blobcache.h"

#define DOMAIN "blobcache"

typedef struct cio_blobcache_t
{
  int dummy;
} cio_blobcache_t;

typedef struct _cache_item_t
{
  time_t created;
  time_t expire;
  size_t size;
} _cache_item_t;

cio_blobcache_t *
cio_blobcache_new()
{
  cio_blobcache_t *cache;
  cache = g_malloc(sizeof(cio_blobcache_t));
  memset(cache, 0, sizeof(cio_blobcache_t));
  return cache;
}

void
cio_blobcache_destroy(cio_blobcache_t *self)
{
  g_free(self);
}

int
cio_blobcache_store(cio_blobcache_t *self, time_t expire, uint32_t hash,
		    const void *data, size_t size)
{
  int fh;
  int res;
  char file[1024];
  char path[512];
  _cache_item_t item;

  /* create cache path */
  snprintf(path, sizeof(path), "%s/blobcache/%.4x",
	   CASTIO_INSTALL_PREFIX"/var/cache/castio", (hash >> 16) & 0xffff);
  mkdir(CASTIO_INSTALL_PREFIX"/var/cache/castio/blobcache", 0700);
  res = mkdir(path, 0700);
  if (res != 0 && errno != EEXIST)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to create directory '%s': %s",
	  path, strerror(errno));
    return -1;
  }

  /* create cache file */
  snprintf(file, sizeof(file), "%s/%.4x", path, (hash & 0xffff));
  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"Storing %lu bytes item into cache file '%s'", size, file);

  /* always truncate file */
  fh = open(file, O_CREAT|O_TRUNC|O_WRONLY, 00600);
  if (fh == -1)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to store cache item '%s': %s", file, strerror(errno));
    return -1;
  }

  /* write header */
  item.created = time(NULL);
  item.expire = expire;
  item.size = size;
  res = write(fh, &item, sizeof(item));
  if (res != sizeof(item))
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to write header of cache item '%s': %s", file, strerror(errno));
    close(fh);
    unlink(file);
    return -1;
  }

  /* write content */
  res = write(fh, data, size);
  if (res != size)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to write content of cache item '%s': %s", file, strerror(errno));
    close(fh);
    unlink(file);
    return -1;
  }

  /* cleanup */
  close(fh);
  return 0;
}

int
cio_blobcache_get(cio_blobcache_t *self, uint32_t hash,
		  void **data, size_t *size)
{
  int fh, res;
  struct stat sb;
  char filename[1024];
  _cache_item_t item;

  snprintf(filename, sizeof(filename), "%s/blobcache/%.4x/%.4x",
	   CASTIO_INSTALL_PREFIX"/var/cache/castio", (hash >> 16) & 0xffff, (hash & 0xffff));
  res = stat(filename, &sb);
  if (res != 0)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to stat cache item '%s': %s", filename, strerror(errno));
    return -1;
  }

  /* read cache file */
  g_log(DOMAIN, G_LOG_LEVEL_INFO,
	"Loading %lu bytes item from cache file '%s'", sb.st_size, filename);


  fh = open(filename, O_RDWR);
  if (fh == -1)
  {
    g_log(DOMAIN, G_LOG_LEVEL_WARNING,
	  "Failed to open cache item '%s': %s", filename, strerror(errno));
    return -1;
  }

  /* read content */
  read(fh, &item, sizeof(item));

  if (size)
    *size = item.size;

  *data = g_malloc(item.size);
  memset(*data, 0, item.size);
  read(fh, *data, item.size);

  /* update header */
  item.created = time(NULL);
  lseek(fh, 0, SEEK_SET);
  write(fh, &item, sizeof(item));
  close(fh);

  return 0;
}


