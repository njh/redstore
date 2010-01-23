#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>

#include <errno.h>
#include <sys/types.h>

#include "redhttpd.h"


void http_headers_send(http_header_t** first, FILE* socket)
{
    http_header_t* it;

    assert(first != NULL);
    assert(socket != NULL);
    
    for (it = *first; it; it = it->next) {
        fprintf(socket, "%s: %s\r\n", it->key, it->value);
    }
}

void http_headers_add(http_header_t** first, const char* key, const char* value)
{
    http_header_t *header = calloc(1, sizeof(http_header_t));
    http_header_t *it;

    assert(first != NULL);
    assert(key != NULL);
    assert(value != NULL);

    // FIXME: append value if header already exists

    header->key = strdup(key);
    header->value = strdup(value);
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

char* http_headers_get(http_header_t** first, const char* key)
{
    http_header_t* it;

    assert(first != NULL);
    assert(key != NULL);
    
    for (it = *first; it; it = it->next) {
        if (strcasecmp(key, it->key)==0)
            return strdup(it->value);
    }

    return NULL;
}


void http_headers_parse_line(http_header_t** first, const char* input)
{
    char *line, *ptr, *key, *value;

    assert(first != NULL);
    assert(input != NULL);
    // FIXME: is there whitespace at the start?
    
    line = strdup(input);
    key = line;
    for (ptr = line; *ptr && *ptr != ':'; ptr++)
        continue;
    *ptr++ = '\0';

    // Skip whitespace
    while(isspace(*ptr))
        ptr++;

    value = ptr;

    http_headers_add(first, key, value);
    
    free(line);
}

void http_headers_free(http_header_t** first)
{
    http_header_t *it, *next;

    assert(first != NULL);

    for (it = *first; it; it = next) {
        next = it->next;
        free(it->key);
        free(it->value);
        free(it);
    }
}
