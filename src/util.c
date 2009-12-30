
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "redstore.h"


char* escape_uri(char *arg)
{ 
    FILE* stream = NULL;
    char *escaped = NULL;
    size_t escaped_size = 0;
    int i; 

    stream = open_memstream(&escaped, &escaped_size);
    if(!stream) return NULL;

    for (i = 0; arg[i]; i++) {
        char c = arg[i];
        if (c == ' ') {
            fputc('+', stream); 
        } else if (
            (c == 0x2D) || (c == 0x2E) || // Hyphen-Minus, Full Stop
            ((0x30 <= c) && (c <= 0x39)) || // Digits [0-9]
            ((0x41 <= c) && (c <= 0x5A)) || // Uppercase [A-Z]
            ((0x61 <= c) && (c <= 0x7A))    // Lowercase [a-z]
        ) { 
            fputc(c, stream); 
        } else {
            fprintf(stream, "%%%02X", c);
        }
    }
    
    fclose(stream);
    
    return escaped;
}


/*
    BSD-compatible implementation of open_memstream 
    Copyright (C) 2005 Richard Kettlewell
*/

#ifndef HAVE_OPEN_MEMSTREAM

struct memstream {
  char *buffer;
  size_t size, space;

  char **ptr;
  size_t *sizeloc;
};

static int memstream_writefn(void *u, const char *buffer, int bytes)
{
  struct memstream *m = u;
  size_t needed = m->size + bytes + 1;
  size_t newspace;
  char *newbuffer;
  
  assert(bytes >= 0);
  if(needed > m->space) {
    newspace = m->space ? m->space : 32;
    while(newspace && newspace < needed)
      newspace *= 2;
    if(!newspace) {
      errno = ENOMEM;
      return -1;
    }
    if(!(newbuffer = realloc(m->buffer, newspace)))
      return -1;
    m->buffer = newbuffer;
    m->space = newspace;
  }
  memcpy(m->buffer + m->size, buffer, bytes);
  m->size += bytes;
  m->buffer[m->size] = 0;
  
  *m->ptr = m->buffer;
  *m->sizeloc = m->size;
  return bytes;
}

FILE *open_memstream(char **ptr, size_t *sizeloc)
{
  struct memstream *m;

  if(!(m = malloc(sizeof *m))) return 0;
  m->buffer = 0;
  m->size = 0;
  m->space = 0;
  m->ptr = ptr;
  m->sizeloc = sizeloc;
  *ptr = 0;
  *sizeloc = 0;
  return funopen(m,
                 0,                     /* read */
                 memstream_writefn,
                 0,                     /* seek */
                 0);                    /* close */
}

#endif

