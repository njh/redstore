/*
    RedStore - a lightweight RDF triplestore powered by Redland
    Copyright (C) 2010 Nicholas J Humfrey <njh@aelius.com>

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
    redstore_page_t *page = NULL;

    // Create the iostream
    page = calloc(1, sizeof(redstore_page_t));
    page->iostream = raptor_new_iostream_to_string(&page->buffer, &page->buffer_len, NULL);
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

    return raptor_iostream_write_string(page->iostream, (unsigned char *) str);
}

int redstore_page_append_decimal(redhttp_response_t * response, int decimal)
{
    redstore_page_t *page = (redstore_page_t *) redhttp_response_get_user_data(response);

    return raptor_iostream_write_decimal(page->iostream, decimal);
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

    return raptor_iostream_write_xml_escaped_string(page->iostream, (unsigned char *) str, str_len,
                                                    quote, NULL, NULL);
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
    redstore_page_append_string(response, "  <li><a href=\"/load\">Load URI</a></li>\n");
    redstore_page_append_string(response, "  <li><a href=\"/data\">Named Graphs</a></li>\n");
    redstore_page_append_string(response, "  <li><a href=\"/dump\">Download Full Dump</a></li>\n");
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
