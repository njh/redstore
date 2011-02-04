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
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "redhttp_private.h"
#include "redhttp.h"


static int hex_decode(const char ch)
{
  int r;
  if ('0' <= ch && ch <= '9')
    r = ch - '0';
  else if ('A' <= ch && ch <= 'F')
    r = ch - 'A' + 0xa;
  else if ('a' <= ch && ch <= 'f')
    r = ch - 'a' + 0xa;
  else
    r = -1;
  return r;
}

char *redhttp_url_unescape(const char *escaped)
{
  char *unescaped, *ptr;
  size_t len = strlen(escaped);
  size_t i;

  // Allocate memory for the new string
  unescaped = calloc(1, len + 1);
  if (unescaped == NULL)
    return NULL;
  ptr = unescaped;

  for (i = 0; i < len; i++) {
    if (escaped[i] == '%') {
      int ch1, ch2;
      if ((i + 3 > len) ||
          (ch1 = hex_decode(escaped[i + 1])) == -1 || (ch2 = hex_decode(escaped[i + 2])) == -1) {
        // Pass invalid escape sequences straight through
        *ptr++ = escaped[i];
      } else {
        // Decode hex
        *ptr++ = ch1 * 16 + ch2;
        i += 2;
      }
    } else if (escaped[i] == '+') {
      *ptr++ = ' ';
    } else {
      *ptr++ = escaped[i];
    }
  }
  *ptr = '\0';
  return unescaped;
}


static int isurlsafe(int c)
{
  return ((unsigned char) (c - 'a') < 26)
      || ((unsigned char) (c - 'A') < 26)
      || ((unsigned char) (c - '0') < 10)
      || (strchr("-._~", c) != NULL);
}

char *redhttp_url_escape(const char *unescaped)
{
  char *escaped, *ptr;
  size_t len = strlen(unescaped);
  size_t enc_len = len;
  size_t i;

  // Phase 1: scan for unsafe characters
  for (i = 0; i < len; i++) {
    if (!isurlsafe(unescaped[i]))
      enc_len += 2;
  }

  // Allocate memory for the new string
  escaped = malloc(enc_len + 1);
  if (escaped == NULL)
    return NULL;
  ptr = escaped;

  // Phase 2: escape any unsafe characters
  for (i = 0; i < len; i++) {
    static const char hex[16] = "0123456789ABCDEF";
    int c = unescaped[i];

    if (isurlsafe(c)) {
      *ptr++ = c;
    } else {
      *ptr++ = '%';
      *ptr++ = hex[c >> 4];
      *ptr++ = hex[c & 0xf];
    }
  }
  *ptr++ = '\0';

  return escaped;
}
