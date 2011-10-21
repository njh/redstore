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

const raptor_syntax_description* redstore_get_format_by_name(description_proc_t desc_proc, const char* format_name)
{
  unsigned int d,n;

  if (!format_name || !format_name[0])
    return NULL;

  for(d=0; 1; d++) {
    const raptor_syntax_description* desc = desc_proc(world, d);
    if (!desc)
      break;

    for (n=0; desc->names[n]; n++) {
      if (strcmp(desc->names[n], format_name) == 0)
        return desc;
    }
  }

  return NULL;
}

const raptor_syntax_description* redstore_negotiate_format(redhttp_request_t * request, description_proc_t desc_proc, const char* default_format, const char** chosen_mime)
{
  const char *format_arg = redhttp_request_get_argument(request, "format");
  const char *accept_str = redhttp_request_get_header(request, "Accept");
  const raptor_syntax_description* chosen_desc = NULL;

  if (format_arg) {
    redstore_debug("format_arg: %s", format_arg);
    chosen_desc = redstore_get_format_by_name(desc_proc, format_arg);
  } else if (accept_str && accept_str[0] && strcmp("*/*", accept_str) != 0) {
    redhttp_negotiate_t *accept = redhttp_negotiate_parse(accept_str);

    if (accept) {
      int best_score = -1;
      unsigned int d,m,a;

      for(d=0; 1; d++) {
        const raptor_syntax_description* desc = desc_proc(world, d);
        if (!desc)
          break;

        for (m=0; m < desc->mime_types_count; m++) {
          const raptor_type_q mime_type = desc->mime_types[m];
          const char* accept_type = NULL;
          int accept_q = 0;
          for (a=0; redhttp_negotiate_get(&accept, a, &accept_type, &accept_q)==0; a++) {
            if (redhttp_negotiate_compare_types(mime_type.mime_type, accept_type)) {
              int score = mime_type.q * accept_q;
              if (score > best_score) {
                best_score = score;
                chosen_desc = desc;
                if (chosen_mime)
                  *chosen_mime = mime_type.mime_type;
              }
            }
          }
        }
      }

      redhttp_negotiate_free(&accept);
    }
  } else if (default_format) {
    redstore_debug("Using default format: %s", default_format);
    chosen_desc = redstore_get_format_by_name(desc_proc, default_format);
  }

  if (chosen_desc) {
    redstore_debug("Chosen format: %s", chosen_desc->label);
    if (chosen_mime && *chosen_mime == NULL && chosen_desc->mime_types)
      *chosen_mime = chosen_desc->mime_types[0].mime_type;
  } else {
    redstore_info("Failed to negotiate a serialisation format");
  }

  return chosen_desc;
}

char *redstore_negotiate_string(redhttp_request_t * request, const char* supported_str, const char* default_format)
{
  const char *format_arg = redhttp_request_get_argument(request, "format");
  const char *accept_str = redhttp_request_get_header(request, "Accept");
  char *format_str = NULL;

  if (format_arg) {
    format_str = calloc(1, strlen(format_arg) + 1);
    if (format_str)
      strcpy(format_str, format_arg);
    redstore_debug("format_arg: %s", format_str);
  } else if (accept_str && accept_str[0] && strcmp("*/*", accept_str) != 0) {
    redhttp_negotiate_t *accept = redhttp_negotiate_parse(accept_str);
    redhttp_negotiate_t *supported = redhttp_negotiate_parse(supported_str);
    format_str = redhttp_negotiate_choose(&supported, &accept);
    redstore_debug("supported: %s", supported_str);
    redstore_debug("accept: %s", accept_str);
    redstore_debug("chosen: %s", format_str);
    redhttp_negotiate_free(&accept);
    redhttp_negotiate_free(&supported);
  } else if (default_format) {
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
