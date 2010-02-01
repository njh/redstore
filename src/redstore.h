#include "config.h"

#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <redland.h>
#include "redhttp/redhttp.h"


#ifndef _REDSTORE_H_
#define _REDSTORE_H_


// ------- Constants -------
#define DEFAULT_PORT            "8080"
#define DEFAULT_STORAGE_NAME    "redstore"
#define DEFAULT_STORAGE_TYPE    "memory"
#define DEFAULT_STORAGE_OPTIONS ""
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

// FIXME: this should be got from Redland
typedef struct serialiser_info {
    const char *name;
    const char *label;
    const char *mime_type;
    const char *uri;
} serialiser_info_t;


// ------- Globals ---------
extern int quiet;
extern int verbose;
extern int running;
extern unsigned long query_count;
extern unsigned long import_count;
extern unsigned long request_count;
extern const char* storage_name;
extern const char* storage_type;
extern librdf_world* world;
extern librdf_storage* storage;
extern librdf_model* model;
extern serialiser_info_t serialiser_info[];


// ------- Prototypes -------

redhttp_response_t* handle_sparql_query(redhttp_request_t *request, void* user_data);
redhttp_response_t* handle_page_home(redhttp_request_t *request, void* user_data);
redhttp_response_t* handle_page_query(redhttp_request_t *request, void* user_data);
redhttp_response_t* handle_page_info(redhttp_request_t *request, void* user_data);
redhttp_response_t* handle_page_formats(redhttp_request_t *request, void* user_data);

redhttp_response_t* redstore_error_page(int level, int code, const char* message);
void page_append_html_header(redhttp_response_t *response, const char* title);
void page_append_html_footer(redhttp_response_t *response);

redhttp_response_t* handle_graph_index(redhttp_request_t *request, void* user_data);
redhttp_response_t* handle_graph_head(redhttp_request_t *request, void* user_data);
redhttp_response_t* handle_graph_get(redhttp_request_t *request, void* user_data);
redhttp_response_t* handle_graph_put(redhttp_request_t *request, void* user_data);
redhttp_response_t* handle_graph_delete(redhttp_request_t *request, void* user_data);

redhttp_response_t* load_stream_into_graph(librdf_stream *stream, librdf_uri *graph_uri);
redhttp_response_t* handle_load_get(redhttp_request_t *request, void* user_data);
redhttp_response_t* handle_load_post(redhttp_request_t *request, void* user_data);

redhttp_response_t* format_bindings_query_result_librdf(redhttp_request_t *request, librdf_query_results* results, const char* format_str);
redhttp_response_t* format_bindings_query_result_html(redhttp_request_t *request, librdf_query_results* results, const char* format_str);
redhttp_response_t* format_bindings_query_result_text(redhttp_request_t *request, librdf_query_results* results, const char* format_str);
redhttp_response_t* format_bindings_query_result(redhttp_request_t *request, librdf_query_results* results);

redhttp_response_t* format_graph_stream_librdf(redhttp_request_t *request, librdf_stream* stream, const char* format_str);
redhttp_response_t* format_graph_stream_html(redhttp_request_t *request, librdf_stream* stream, const char* format_str);
redhttp_response_t* format_graph_stream_text(redhttp_request_t *request, librdf_stream* stream, const char* format_str);
redhttp_response_t* format_graph_stream(redhttp_request_t *request, librdf_stream* stream);

redhttp_response_t* handle_image_favicon(redhttp_request_t *request, void* user_data);

void redstore_log( RedstoreLogLevel level, const char* fmt, ... );



#endif
