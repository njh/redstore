#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "redhttpd.h"


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

char* http_url_unescape(const char* escaped)
{
	char* unescaped = strdup(escaped);
	char* ptr = unescaped;
	size_t len = strlen(escaped);
	int i;

	for(i=0; i < len; i++) {
    	if (escaped[i] == '%') {
			int ch1, ch2;
			if ((i+3 > len) ||
			    (ch1 = hex_decode(escaped[i + 1])) == -1 || 
				(ch2 = hex_decode(escaped[i + 2])) == -1)
			{
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


char* http_url_escape(const char *arg)
{ 
	// FIXME: implement this
	return strdup(arg);
}


/*
char* http_url_escape(char *arg)
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
*/

