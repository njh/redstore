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

redhttp_response_t *redstore_error_page(int level, int code, const char *message)
{
  redstore_log(level, message);
  return redhttp_response_new_error_page(code, message);
}

redhttp_response_t *redstore_page_new(const char *title)
{
  redhttp_response_t *response = redhttp_response_new_with_type(REDHTTP_OK, NULL, "text/html");
  raptor_world *raptor = librdf_world_get_raptor(world);
  redstore_page_t *page = NULL;

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
  redhttp_response_set_content(response, page->buffer, page->buffer_len);

  free(page);
}

redhttp_response_t *handle_page_home(redhttp_request_t * request, void *user_data)
{
  redhttp_response_t *response = redstore_page_new("RedStore");
  redstore_page_append_string(response, "<ul>\n");
  redstore_page_append_string(response, "  <li><a href=\"/query\">Query Form</a></li>\n");
  redstore_page_append_string(response, "  <li><a href=\"/graphs\">List Named Graphs</a></li>\n");
  redstore_page_append_string(response, "  <li>Update:<ul>");
  redstore_page_append_string(response, "    <li><a href=\"/load\">Load URI</a></li>");
  redstore_page_append_string(response, "    <li><a href=\"/insert\">Insert Triples</a></li>");
  redstore_page_append_string(response, "    <li><a href=\"/delete\">Delete Triples</a></li>");
  redstore_page_append_string(response, "  </ul></li>\n");
  redstore_page_append_string(response,
                              "  <li><a href=\"/data?format=nquads\">Download Dump</a></li>\n");
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
  redhttp_response_copy_content(response, text, sizeof(text));
  return response;
}


static void syntax_select_list(const char *name, const char *default_name, librdf_node * source,
                               librdf_node * arc, redhttp_response_t * response)
{
  librdf_iterator *iterator;

  redstore_page_append_strings(response, "<select name=\"", name, "\">\n", NULL);
  iterator = librdf_model_get_targets(sd_model, source, arc);
  while (!librdf_iterator_end(iterator)) {
    librdf_node *format_node = librdf_iterator_get_object(iterator);
    if (format_node) {
      librdf_node *name = librdf_model_get_target(sd_model, format_node, LIBRDF_S_label(world));
      librdf_node *desc = librdf_model_get_target(sd_model, format_node, LIBRDF_S_comment(world));
      if (name && desc) {
        const char *name_str = (char *) librdf_node_get_literal_value(name);
        redstore_page_append_strings(response, "<option value=\"", name_str, "\"", NULL);
        if (strcmp(name_str, default_name) == 0)
          redstore_page_append_string(response, " selected=\"selected\"");
        redstore_page_append_string(response, ">");
        redstore_page_append_escaped(response, (char *) librdf_node_get_literal_value(desc), 0);
        redstore_page_append_string(response, "</option>\n");
      }
    }
    librdf_iterator_next(iterator);
  }
  librdf_free_iterator(iterator);
  redstore_page_append_string(response, "</select>\n");
}



redhttp_response_t *handle_page_query_form(redhttp_request_t * request, void *user_data)
{
  librdf_node *ql_node = librdf_new_node_from_uri_local_name(world, saddle_ns_uri,
                                                             (unsigned char *) "queryLanguage");
  librdf_node *rf_node =
      librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char *) "resultFormat");
  redhttp_response_t *response = NULL;

  response = redstore_page_new("Query Form");
  redstore_page_append_string(response, "<form action=\"./query\" method=\"get\">\n");
  redstore_page_append_string(response, "<div><textarea name=\"query\" cols=\"80\" rows=\"18\">\n");
  redstore_page_append_string(response,
                              "PREFIX rdf: &lt;http://www.w3.org/1999/02/22-rdf-syntax-ns#&gt;\n");
  redstore_page_append_string(response,
                              "PREFIX rdfs: &lt;http://www.w3.org/2000/01/rdf-schema#&gt;\n\n");
  redstore_page_append_string(response, "SELECT * WHERE {\n ?s ?p ?o\n} LIMIT 10\n");
  redstore_page_append_string(response, "</textarea></div>\n");

  redstore_page_append_string(response, "<div class=\"buttons\">\n");
  redstore_page_append_string(response, "Query Language: ");
  syntax_select_list("lang", DEFAULT_QUERY_LANGUAGE, service_node, ql_node, response);
  redstore_page_append_string(response, "<br />");

  redstore_page_append_string(response, "Result Format: ");
  syntax_select_list("format", "html", service_node, rf_node, response);
  redstore_page_append_string(response, "<br />");

  redstore_page_append_string(response, "<input type=\"reset\" /> ");
  redstore_page_append_string(response, "<input type=\"submit\" value=\"Execute\" />\n");
  redstore_page_append_string(response, "</div></form>\n");
  redstore_page_end(response);

  librdf_free_node(ql_node);
  librdf_free_node(rf_node);

  return response;
}

redhttp_response_t *handle_page_update_form(redhttp_request_t * request, void *user_data)
{
  librdf_node *pf_node =
      librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char *) "parseFormat");
  const char *title = (char *) user_data;
  const char *action = redhttp_request_get_path(request);

  redhttp_response_t *response = redstore_page_new(title);
  redstore_page_append_strings(response, "<form method=\"post\" action=\"", action, "\">\n", NULL);
  redstore_page_append_string(response,
                              "<div><textarea name=\"content\" cols=\"80\" rows=\"18\">\n");
  redstore_page_append_string(response,
                              "&lt;http://example.com/&gt; &lt;http://purl.org/dc/terms/title&gt; \"Example Web Page\" .\n");
  redstore_page_append_string(response, "</textarea></div>\n");
  redstore_page_append_string(response, "<div class=\"buttons\">\n");
  redstore_page_append_string(response, "Triples syntax: ");
  syntax_select_list("content-type", DEFAULT_PARSE_FORMAT, service_node, pf_node, response);
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
  redhttp_response_t *response = redstore_page_new("Load URI");
  redstore_page_append_string(response, "<form method=\"post\" action=\"/load\"><div>\n"
                              "<label for=\"uri\">URI:</label> <input id=\"uri\" name=\"uri\" type=\"text\" size=\"40\" /><br />\n"
                              "<label for=\"graph\">Graph:</label> <input id=\"graph\" name=\"graph\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
                              "<label for=\"base-uri\">Base URI:</label> <input id=\"base-uri\" name=\"base-uri\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
                              "<input type=\"reset\" /> <input type=\"submit\" />\n"
                              "</div></form>\n");

  redstore_page_end(response);

  return response;
}
