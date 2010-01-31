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

#include "redhttp.h"


void redhttp_headers_send(redhttp_header_t** first, FILE* socket)
{
    redhttp_header_t* it;

    assert(first != NULL);
    assert(socket != NULL);
    
    for (it = *first; it; it = it->next) {
        fprintf(socket, "%s: %s\r\n", it->key, it->value);
    }
}

void redhttp_headers_add(redhttp_header_t** first, const char* key, const char* value)
{
    redhttp_header_t *header;
    redhttp_header_t *it;

    assert(first != NULL);
    assert(key != NULL);
    assert(strlen(key) > 0);
    assert(value != NULL);
    
    if (strlen(value) == 0) return;

    // FIXME: append value if header already exists

	// Create new header item
	header = calloc(1, sizeof(redhttp_header_t));
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

char* redhttp_headers_get(redhttp_header_t** first, const char* key)
{
    redhttp_header_t* it;

    assert(first != NULL);
    assert(key != NULL);
    
    for (it = *first; it; it = it->next) {
        if (strcasecmp(key, it->key)==0)
            return strdup(it->value);
    }

    return NULL;
}


void redhttp_headers_parse_line(redhttp_header_t** first, const char* input)
{
    char *line, *ptr, *key, *value;

    assert(first != NULL);
    assert(input != NULL);
    if (strlen(input) < 1) return;

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

    redhttp_headers_add(first, key, value);
    
    free(line);
}

void redhttp_headers_free(redhttp_header_t** first)
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
