/*
    RedHTTP - a lightweight HTTP server library
    Copyright (C) 2010-2011 Nicholas J Humfrey <njh@aelius.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include <errno.h>
#include <sys/types.h>

#include "redhttp_private.h"
#include "redhttp.h"


static int redhttp_strcasecmp(const char *s1, const char *s2)
{
  int r = 0;

  while (((s1 == s2) || !(r = ((int) (tolower(*((char *) s1))))
                          - tolower(*((char *) s2))))
         && (++s2, *s1++));

  return r;
}

void redhttp_headers_print(redhttp_header_t ** first, FILE * socket)
{
  redhttp_header_t *it;

  assert(first != NULL);
  assert(socket != NULL);

  for (it = *first; it; it = it->next) {
    fprintf(socket, "%s: %s\r\n", it->key, it->value);
  }
}

void redhttp_headers_add(redhttp_header_t ** first, const char *key, const char *value)
{
  redhttp_header_t *header;
  redhttp_header_t *it;

  assert(first != NULL);
  assert(key != NULL);
  assert(strlen(key) > 0);
  assert(value != NULL);

  if (strlen(value) == 0)
    return;

  // FIXME: append value if header already exists

  // Create new header item
  header = calloc(1, sizeof(redhttp_header_t));
  header->key = redhttp_strdup(key);
  header->value = redhttp_strdup(value);
  header->next = NULL;

  // append the new method to the list
  if (*first) {
    // get to the last member of the list
    for (it = *first; it->next; it = it->next);
    it->next = header;
  } else {
    *first = header;
  }
}

int redhttp_headers_count(redhttp_header_t ** first)
{
  redhttp_header_t *it;
  int count = 0;

  assert(first != NULL);

  for (it = *first; it; it = it->next) {
    count++;
  }

  return count;
}

const char *redhttp_headers_get(redhttp_header_t ** first, const char *key)
{
  redhttp_header_t *it;

  assert(first != NULL);
  assert(key != NULL);

  for (it = *first; it; it = it->next) {
    if (redhttp_strcasecmp(key, it->key) == 0)
      return it->value;
  }

  return NULL;
}


void redhttp_headers_parse_line(redhttp_header_t ** first, const char *input)
{
  char *line, *ptr, *key, *value;

  assert(first != NULL);
  assert(input != NULL);
  if (strlen(input) < 1)
    return;

  // FIXME: is there whitespace at the start?

  line = redhttp_strdup(input);
  if (!line)
    return;
  key = line;
  for (ptr = line; *ptr && *ptr != ':'; ptr++)
    continue;
  if (!*ptr) {
    free(line);
    return;
  }
  *ptr++ = '\0';

  // Skip whitespace
  while (isspace(*ptr))
    ptr++;

  value = ptr;

  if (*value != '\0') {
    redhttp_headers_add(first, key, value);
  }

  free(line);
}

void redhttp_headers_free(redhttp_header_t ** first)
{
  redhttp_header_t *it, *next;

  assert(first != NULL);

  for (it = *first; it; it = next) {
    next = it->next;
    free(it->key);
    free(it->value);
    free(it);
  }
}
