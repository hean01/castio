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

#ifndef _blobcache_h
#define _blobcache_h

#include <inttypes.h>

struct cio_blobcache_t;

typedef struct cio_blobcache_resource_header_t
{
  char mime[64];
  uint32_t size;
} cio_blobcache_resource_header_t;

struct cio_blobcache_t *cio_blobcache_new();
void cio_blobcache_destroy(struct cio_blobcache_t *self);

int cio_blobcache_store(struct cio_blobcache_t *self, time_t expire, uint32_t hash,
			const void *data, size_t size);

int cio_blobcache_get(struct cio_blobcache_t *self, uint32_t hash,
		      void **data, size_t *size);


#endif /* _blobcache_h */
