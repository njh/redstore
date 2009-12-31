#include "config.h"

#include <stdarg.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#include <microhttpd.h>
#include <redland.h>


#ifndef _REDSTORE_H_
#define _REDSTORE_H_


// ------- Constants -------
#define DEFAULT_PORT    (8080)
#define DEFAULT_STORAGE_NAME    "redstore"
#define DEFAULT_STORAGE_TYPE    "hashes"
#define DEFAULT_STORAGE_OPTIONS "hash-type='bdb',dir='.'"
#define DEFAULT_RESULTS_FORMAT  "application/sparql-results+xml"


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

// FIXME: ICK
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

// FIXME: ICK ICK
typedef struct http_response {
    struct MHD_Response *mhd_response;
    unsigned int status;
} http_response_t;


// ------- Globals ---------
extern librdf_world* world;
extern librdf_storage* storage;
extern librdf_model* model;



// ------- Prototypes -------

http_response_t* new_http_response(http_request_t *request, unsigned int status,
                       void* content, size_t content_size,
                       const char* content_type);

http_response_t* handle_sparql_query(http_request_t *request);
http_response_t* handle_homepage(http_request_t *request);
http_response_t* handle_formats_page(http_request_t *request);
http_response_t* handle_querypage(http_request_t *request);
http_response_t* handle_graph_index(http_request_t *request);
http_response_t* handle_graph_show(http_request_t *request, char* url);
http_response_t* handle_error(http_request_t *request, unsigned int status);
http_response_t* handle_redirect(http_request_t *request, char* url);
http_response_t* handle_html_page(http_request_t *request, unsigned int status, 
                     const char* title, char* page);

http_response_t* handle_favicon(http_request_t *request);

void redstore_log( RedstoreLogLevel level, const char* fmt, ... );

char* escape_uri(char *arg);

#ifndef HAVE_OPEN_MEMSTREAM
FILE *open_memstream(char **ptr, size_t *sizeloc);
#endif


#endif
