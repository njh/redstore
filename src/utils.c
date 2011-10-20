/*
    RedStore - a lightweight RDF triplestore powered by Redland
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
#include <assert.h>
#include <stdarg.h>

#include "redstore.h"


void redstore_log(librdf_log_level level, const char *fmt, ...)
{
  time_t t = time(NULL);
  char *time_str;
  va_list args;

  // Display the message level
  switch(level) {
    case LIBRDF_LOG_DEBUG:
      if (!verbose)
        return;
      printf("[DEBUG]   ");
    break;
    case LIBRDF_LOG_INFO:
      if (quiet)
        return;
      printf("[INFO]    ");
    break;
    case LIBRDF_LOG_WARN:
      printf("[WARNING] ");
    break;
    case LIBRDF_LOG_ERROR:
      printf("[ERROR]   ");
    break;
    case LIBRDF_LOG_FATAL:
      printf("[FATAL]   ");
    break;
    default:
      printf("[UNKNOWN] ");
    break;
  }

  // Display timestamp
  time_str = ctime(&t);
  time_str[strlen(time_str) - 1] = 0; // remove \n
  printf("%s  ", time_str);

  // Display the error message
  va_start(args, fmt);
  vprintf(fmt, args);
  printf("\n");
  va_end(args);

  // If fatal then stop
  if (level == LIBRDF_LOG_FATAL) {
    // Exit with a non-zero exit code if there was a fatal error
    exit_code++;
    if (running) {
      // Quit gracefully
      running = 0;
    } else {
      fprintf(stderr, "Fatal error while quiting; exiting immediately.");
      exit(-1);
    }
  }
}

char *redstore_get_format(redhttp_request_t * request, redhttp_negotiate_t * supported, const char* default_format)
{
  const char *format_arg;
  char *format_str = NULL;

  format_arg = redhttp_request_get_argument(request, "format");
  if (format_arg) {
    format_str = calloc(1, strlen(format_arg) + 1);
    if (format_str)
      strcpy(format_str, format_arg);
    redstore_debug("format_arg: %s", format_str);
  }

  if (!format_str) {
    const char *accept_str = redhttp_request_get_header(request, "Accept");
    if (accept_str && accept_str[0] && strcmp("*/*", accept_str) != 0) {
      redhttp_negotiate_t *accept = redhttp_negotiate_parse(accept_str);
      char *supported_str = redhttp_negotiate_to_string(&supported);
      format_str = redhttp_negotiate_choose(&supported, &accept);
      redstore_debug("supported: %s", supported_str);
      redstore_debug("accept: %s", accept_str);
      redstore_debug("chosen: %s", format_str);
      redhttp_negotiate_free(&accept);
      free(supported_str);
    }
  }

  if (!format_str) {
    format_str = calloc(1, strlen(default_format) + 1);
    if (format_str)
      strcpy(format_str, default_format);
    redstore_debug("Using default format: %s", default_format);
  }

  return format_str;
}

int redstore_is_html_format(const char *str)
{
  if (strcmp(str, "html") == 0 ||
      strcmp(str, "text/html") == 0 || strcmp(str, "application/xhtml+xml") == 0)
    return 1;
  else
    return 0;
}

int redstore_is_text_format(const char *str)
{
  if (strcmp(str, "text") == 0 || strcmp(str, "text/plain") == 0)
    return 1;
  else
    return 0;
}

int redstore_is_nquads_format(const char *str)
{
  if (strcmp(str, "nquads") == 0 || strcmp(str, "application/x-nquads") == 0)
    return 1;
  else
    return 0;
}
