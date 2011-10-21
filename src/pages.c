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

typedef struct redstore_page_s {
  const char *title;
  void *buffer;
  size_t buffer_len;
  raptor_iostream *iostream;
} redstore_page_t;

redhttp_response_t *redstore_page_new(int code, const char *title)
{
  raptor_world *raptor = librdf_world_get_raptor(world);
  redhttp_response_t *response = NULL;
  redstore_page_t *page = NULL;

  if (!code)
    code = REDHTTP_OK;

  response = redhttp_response_new_with_type(code, NULL, "text/html");
  if (!response)
    return NULL;

  if (!title)
    title = redhttp_response_get_status_message(response);

  // Create the iostream
  page = calloc(1, sizeof(redstore_page_t));
  page->iostream = raptor_new_iostream_to_string(raptor, &page->buffer, &page->buffer_len, NULL);
  page->title = title;
  redhttp_response_set_user_data(response, page);

  redstore_page_append_string(response, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
  redstore_page_append_string(response,
                              "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n");
  redstore_page_append_string(response,
                              "          \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
  redstore_page_append_string(response, "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n");
  redstore_page_append_string(response, "<head>\n");
  redstore_page_append_strings(response, "  <title>RedStore - ", title, "</title>\n", NULL);
  redstore_page_append_string(response, "</head>\n");
  redstore_page_append_string(response, "<body>\n");
  redstore_page_append_strings(response, "<h1>", title, "</h1>\n", NULL);

  return response;
}

redhttp_response_t *redstore_page_new_with_message(redhttp_request_t *request, int log_level, int code, const char *format, ...)
{
  char *format_str = redstore_negotiate_string(request, "text/plain,text/html,application/xhtml+xml", "text/plain");
  const char * title = redhttp_response_status_message_for_code(code);
  redhttp_response_t *response = NULL;
  size_t message_len = 0;
  char* message = NULL;
  va_list ap;

  assert(request != NULL);
  assert(format != NULL);

  // Work out how long the full message is
  va_start(ap, format);
  message_len = vsnprintf(NULL, 0, format, ap);
  va_end(ap);

  // Allocate memory for the message
  message = malloc(message_len+1);
  if (!message)
    return NULL;

  // Create the full message
  va_start(ap, format);
  vsnprintf(message, message_len+1, format, ap);
  va_end(ap);

  if (redstore_is_html_format(format_str)) {
    response = redstore_page_new(code, title);
    redstore_page_append_string(response, "<p>");
    redstore_page_append_escaped(response, message, 0);
    redstore_page_append_string(response, "</p>\n");

    if (error_buffer) {
      redstore_page_append_string(response, "<pre>");
      redstore_page_append_string_buffer(response, error_buffer, 1);
      redstore_page_append_string(response, "</pre>");
    }
    redstore_page_end(response);
  } else {
    const char* error_str = "";
    size_t text_len = strlen(message) + 2;
    char *text = NULL;

    if (error_buffer) {
      text_len += raptor_stringbuffer_length(error_buffer);
      error_str = (const char*)raptor_stringbuffer_as_string(error_buffer);
    }

    text = malloc(text_len);
    if (text) {
      response = redhttp_response_new_with_type(code, NULL, "text/plain");
      snprintf(text, text_len, "%s\n%s", message, error_str);
      redhttp_response_set_content(response, text, text_len-1, free);
    }
  }

  if (log_level) {
    redstore_log(log_level, "Response: %s - %s", title, message);
  }

  if (format_str)
    free(format_str);
  if (message)
    free(message);

  return response;
}

int redstore_page_append_string(redhttp_response_t * response, const char *str)
{
  redstore_page_t *page = (redstore_page_t *) redhttp_response_get_user_data(response);

  return raptor_iostream_string_write((unsigned char *) str, page->iostream);
}

int redstore_page_append_decimal(redhttp_response_t * response, int decimal)
{
  redstore_page_t *page = (redstore_page_t *) redhttp_response_get_user_data(response);

  return raptor_iostream_decimal_write(decimal, page->iostream);
}

int redstore_page_append_strings(redhttp_response_t * response, ...)
{
  redstore_page_t *page = (redstore_page_t *) redhttp_response_get_user_data(response);
  va_list ap;
  char *str;

  va_start(ap, response);
  while ((str = va_arg(ap, char *)) != NULL) {
    raptor_iostream_string_write((unsigned char *) str, page->iostream);
  }
  va_end(ap);

  return 0;
}

int redstore_page_append_string_buffer(redhttp_response_t * response, raptor_stringbuffer *buffer, int escape)
{
  const char* string = (const char*)raptor_stringbuffer_as_string(buffer);
  if (escape) {
    return redstore_page_append_escaped(response, string, 0);
  } else {
    return redstore_page_append_string(response, string);
  }
}

int redstore_page_append_escaped(redhttp_response_t * response, const char *str, char quote)
{
  redstore_page_t *page = (redstore_page_t *) redhttp_response_get_user_data(response);
  size_t str_len = strlen(str);

  return raptor_xml_escape_string_write((unsigned char *) str, str_len, quote, page->iostream);
}

void redstore_page_end(redhttp_response_t * response)
{
  redstore_page_t *page = (redstore_page_t *) redhttp_response_get_user_data(response);

  // Footer
  redstore_page_append_string(response, "<hr /><address>");
  redstore_page_append_string(response, PACKAGE_NAME "/" PACKAGE_VERSION);
  redstore_page_append_strings(response, " librdf/", librdf_version_string, NULL);
  redstore_page_append_strings(response, " raptor/", raptor_version_string, NULL);
  redstore_page_append_strings(response, " rasqal/", rasqal_version_string, NULL);
  redstore_page_append_string(response, "</address>\n");
  redstore_page_append_string(response, "</body></html>\n");

  raptor_free_iostream(page->iostream);
  redhttp_response_set_content(response, page->buffer, page->buffer_len, free);

  free(page);
}

redhttp_response_t *handle_page_home(redhttp_request_t * request, void *user_data)
{
  redhttp_response_t *response = redstore_page_new(REDHTTP_OK, "RedStore");
  redstore_page_append_string(response, "<ul>\n");
  redstore_page_append_string(response, "  <li><a href=\"/query\">Query Form</a></li>\n");
  redstore_page_append_string(response, "  <li><a href=\"/graphs\">List Named Graphs</a></li>\n");
  redstore_page_append_string(response, "  <li>Update:<ul>");
  redstore_page_append_string(response, "    <li><a href=\"/load\">Load URI</a></li>");
  redstore_page_append_string(response, "    <li><a href=\"/insert\">Insert Triples</a></li>");
  redstore_page_append_string(response, "    <li><a href=\"/delete\">Delete Triples</a></li>");
  redstore_page_append_string(response, "  </ul></li>\n");
  redstore_page_append_string(response,
                              "  <li><a href=\"/data?default&amp;format=nquads\">Download Dump</a></li>\n");
  redstore_page_append_string(response,
                              "  <li><a href=\"/description\">Service Description</a></li>\n");
  redstore_page_append_string(response, "</ul>\n");
  redstore_page_end(response);

  return response;
}

redhttp_response_t *handle_page_robots_txt(redhttp_request_t * request, void *user_data)
{
  redhttp_response_t *response = redhttp_response_new_with_type(REDHTTP_OK, NULL, "text/plain");
  static const char text[] = "User-agent: *\nDisallow: /\n";
  redhttp_response_set_content(response, (char*)text, sizeof(text)-1, NULL);
  return response;
}


static void syntax_select_list(const char *field_name, const char *default_name,
                               description_proc_t desc_proc, redhttp_response_t * response)
{
  int i;

  if (field_name)
    redstore_page_append_strings(response, "<select name=\"", field_name, "\">\n", NULL);

  for(i=0; 1; i++) {
    const raptor_syntax_description* desc = desc_proc(world, i);
    if (!desc)
      break;

    if (desc->names && desc->names[0] && desc->label) {
      const char *name_str = desc->names[0];
      redstore_page_append_strings(response, "<option value=\"", name_str, "\"", NULL);
      if (default_name && strcmp(name_str, default_name) == 0)
        redstore_page_append_string(response, " selected=\"selected\"");
      redstore_page_append_string(response, ">");
      redstore_page_append_escaped(response, desc->label, 0);
      redstore_page_append_string(response, "</option>\n");
    }
  }

  if (field_name)
    redstore_page_append_string(response, "</select>\n");
}



redhttp_response_t *handle_page_query_form(redhttp_request_t * request, void *user_data)
{
  redhttp_response_t *response = NULL;

  response = redstore_page_new(REDHTTP_OK, "Query Form");
  redstore_page_append_string(response, "<form action=\"./query\" method=\"get\">\n");
  redstore_page_append_string(response, "<div><textarea name=\"query\" cols=\"80\" rows=\"18\">\n");
  redstore_page_append_string(response,
                              "PREFIX rdf: &lt;http://www.w3.org/1999/02/22-rdf-syntax-ns#&gt;\n");
  redstore_page_append_string(response,
                              "PREFIX rdfs: &lt;http://www.w3.org/2000/01/rdf-schema#&gt;\n\n");
  redstore_page_append_string(response, "SELECT * WHERE {\n GRAPH ?g {?s ?p ?o}\n} LIMIT 10\n");
  redstore_page_append_string(response, "</textarea></div>\n");

  redstore_page_append_string(response, "<div class=\"buttons\">\n");
  redstore_page_append_string(response, "Query Language: ");
  syntax_select_list("lang", DEFAULT_QUERY_LANGUAGE, librdf_query_language_get_description, response);
  redstore_page_append_string(response, "<br />");

  redstore_page_append_string(response, "Result Format: ");
  redstore_page_append_string(response, "<select name=\"format\">\n");

  redstore_page_append_string(response, "<optgroup label=\"Query Results Formats\">\n");
  syntax_select_list(NULL, "html", librdf_query_results_formats_get_description, response);
  redstore_page_append_string(response, "</optgroup>\n");

  redstore_page_append_string(response, "<optgroup label=\"RDF Formats\">\n");
  syntax_select_list(NULL, NULL, librdf_serializer_get_description, response);
  redstore_page_append_string(response, "</optgroup>\n");

  redstore_page_append_string(response, "</select>\n");
  redstore_page_append_string(response, "<br />");

  redstore_page_append_string(response, "<input type=\"reset\" /> ");
  redstore_page_append_string(response, "<input type=\"submit\" value=\"Execute\" />\n");
  redstore_page_append_string(response, "</div></form>\n");
  redstore_page_end(response);

  return response;
}

redhttp_response_t *handle_page_update_form(redhttp_request_t * request, void *user_data)
{
  const char *title = (char *) user_data;
  const char *action = redhttp_request_get_path(request);

  redhttp_response_t *response = redstore_page_new(REDHTTP_OK, title);
  redstore_page_append_strings(response, "<form method=\"post\" action=\"", action, "\">\n", NULL);
  redstore_page_append_string(response,
                              "<div><textarea name=\"content\" cols=\"80\" rows=\"18\">\n");
  redstore_page_append_string(response,
                              "&lt;http://example.com/&gt; &lt;http://purl.org/dc/terms/title&gt; \"Example Web Page\" .\n");
  redstore_page_append_string(response, "</textarea></div>\n");
  redstore_page_append_string(response, "<div class=\"buttons\">\n");
  redstore_page_append_string(response, "Triples syntax: ");
  syntax_select_list("content-type", DEFAULT_PARSE_FORMAT, librdf_parser_get_description, response);
  redstore_page_append_string(response, "<br />");
  redstore_page_append_string(response,
                              "<label for=\"graph\">Graph:</label> <input id=\"graph\" name=\"graph\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
                              "<label for=\"base-uri\">Base URI:</label> <input id=\"base-uri\" name=\"base-uri\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
                              "<input type=\"reset\" /> <input type=\"submit\" />\n"
                              "</div></form>\n");
  redstore_page_end(response);

  return response;
}


redhttp_response_t *handle_page_load_form(redhttp_request_t * request, void *user_data)
{
  redhttp_response_t *response = redstore_page_new(REDHTTP_OK, "Load URI");
  redstore_page_append_string(response, "<form method=\"post\" action=\"/load\"><div>\n"
                              "<label for=\"uri\">URI:</label> <input id=\"uri\" name=\"uri\" type=\"text\" size=\"40\" /><br />\n"
                              "<label for=\"graph\">Graph:</label> <input id=\"graph\" name=\"graph\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
                              "<label for=\"base-uri\">Base URI:</label> <input id=\"base-uri\" name=\"base-uri\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
                              "<input type=\"reset\" /> <input type=\"submit\" />\n"
                              "</div></form>\n");

  redstore_page_end(response);

  return response;
}
