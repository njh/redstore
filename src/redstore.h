#include "config.h"

#include <stdarg.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>

#include <microhttpd.h>


#ifndef _REDSTORE_H_
#define _REDSTORE_H_


// ------- Constants -------
#define DEFAULT_PORT    (8080)




// ------- Logging ---------
typedef enum {
    REDSTORE_DEBUG=1,   // Only display debug if verbose
    REDSTORE_INFO,      // Don't show info when quiet
    REDSTORE_ERROR,     // Always display
    REDSTORE_FATAL      // Quit if fatal
} RedstoreLogLevel;


#define redstore_debug( ... ) \
		redstore_log(REDSTORE_DEBUG, __VA_ARGS__ )

#define redstore_info( ... ) \
		redstore_log(REDSTORE_INFO, __VA_ARGS__ )

#define redstore_error( ... ) \
		redstore_log( REDSTORE_ERROR, __VA_ARGS__ )

#define redstore_fatal( ... ) \
		redstore_log( REDSTORE_FATAL, __VA_ARGS__ )



// ------- Structures -------

typedef struct http_request {
    void *cls;
    struct MHD_Connection *connection;
    const char *url;
    const char *method;
    const char *version;
    const char *upload_data;
    size_t *upload_data_size;
    void **con_cls;
} http_request_t;


typedef struct MHD_Response http_response_t;


// ------- Globals ---------



// ------- Prototypes -------

int handle_homepage(http_request_t *request);
int handle_favicon(http_request_t *request);
int handle_error(http_request_t *request, unsigned int status);
int handle_html_page(http_request_t *request, unsigned int status, 
                     const char* title, const char* page);
int handle_static_data(http_request_t *request, unsigned int status,
                       void* content, size_t content_size,
                       const char* content_type);

void redstore_log( RedstoreLogLevel level, const char* fmt, ... );


#endif
