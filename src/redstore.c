#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include "redstore.h"


// ------- Globals -------
int quiet = 0;			// Only display error messages
int verbose = 0;		// Increase number of logging messages
int running = 1;        // True while still running
unsigned long query_count = 0;
unsigned long import_count = 0;
unsigned long request_count = 0;
const char* storage_name = DEFAULT_STORAGE_NAME;
const char* storage_type = DEFAULT_STORAGE_TYPE;
librdf_world* world = NULL;
librdf_model* model = NULL;
librdf_storage* storage = NULL;

// FIXME: this should be got from Redland
serialiser_info_t serialiser_info[] = {
    { .name = "ntriples", .label = "N-Triples", .mime_type = "application/x-ntriples", .uri = "http://www.w3.org/TR/rdf-testcases/#ntriples" },
    { .name = "turtle", .label = "Turtle", .mime_type = "application/x-turtle", .uri = "http://www.dajobe.org/2004/01/turtle" },
    { .name = "turtle", .label = NULL, .mime_type = "text/turtle", .uri = "http://www.dajobe.org/2004/01/turtle" },
    { .name = "rdfxml-abbrev", .label = "RDF/XML (Abbreviated)", .mime_type = "application/rdf+xml", .uri = "http://www.w3.org/TR/rdf-syntax-grammar" },
    { .name = "rdfxml", .label = "RDF/XML", .mime_type = "application/rdf+xml", .uri = "http://www.w3.org/TR/rdf-syntax-grammar" },
    { .name = "dot", .label = "GraphViz DOT format", .mime_type = "text/x-graphviz", .uri = "http://www.graphviz.org/doc/info/lang.html" },
    { .name = "json", .label = "RDF/JSON Resource-Centric", .mime_type = "application/json", .uri = "http://n2.talis.com/wiki/RDF_JSON_Specification" },
    { .name = "json-triples", .label = "RDF/JSON Triples", .mime_type = "application/json", .uri = NULL },
    { .name = NULL }
};


static void termination_handler (int signum)
{
    switch(signum) {
        case SIGHUP:    redstore_info("Got hangup signal."); break;
        case SIGTERM:   redstore_info("Got termination signal."); break;
        case SIGINT:    redstore_info("Got interupt signal."); break;
    }
    
    signal(signum, termination_handler);
    
    // Signal the main thead to stop
    running = 0;
}


void redstore_log( RedstoreLogLevel level, const char* fmt, ... )
{
    time_t t=time(NULL);
    char time_str[32];
    va_list args;
    
    
    // Display the message level
    if (level == REDSTORE_DEBUG ) {
        if (!verbose) return;
        printf( "[DEBUG]  " );
    } else if (level == REDSTORE_INFO ) {
        if (quiet) return;
        printf( "[INFO]   " );
    } else if (level == REDSTORE_ERROR ) {
        printf( "[ERROR]  " );
    } else if (level == REDSTORE_FATAL ) {
        printf( "[FATAL]  " );
    } else {
        printf( "[UNKNOWN]" );
    }

    // Display timestamp
    ctime_r( &t, time_str );
    time_str[strlen(time_str)-1]=0; // remove \n
    printf( "%s  ", time_str );
    
    // Display the error message
    va_start( args, fmt );
    vprintf( fmt, args );
    printf( "\n" );
    va_end( args );
    
    // If fatal then stop
    if (level == REDSTORE_FATAL) {
        if (running) {
            // Quit gracefully
            running = 0;
        } else {
            printf( "Fatal error while quiting; exiting immediately." );
            exit(-1);
        }
    }
}

static http_response_t* request_counter(http_request_t *request, void* user_data)
{
	request_count++;
	return NULL;
}

static int redland_log_handler(void* user, librdf_log_message* msg)
{
    int level = librdf_log_message_level(msg);
    int code = librdf_log_message_code(msg);
    const char* message = librdf_log_message_message(msg);
    printf("redland_log_handler: code=%d level=%d message=%s\n", code, level, message);
    return 0;
}


// Display how to use this program
static void usage()
{
    int i;
    printf("%s version %s\n\n", PACKAGE_NAME, PACKAGE_VERSION);
    printf("Usage: %s [options] [<name>]\n", PACKAGE_TARNAME);
    printf("   -p <port>       Port number to run HTTP server on (default %s)\n", DEFAULT_PORT);
    printf("   -s <type>       Set the graph storage type\n");
    for(i=0; 1; i++) {
        const char *help_name;
        const char *help_label;
        if(librdf_storage_enumerate(world, i, &help_name, &help_label))
            break;
        printf("      %-10s     %s", help_name, help_label);
        if(strcmp(help_name,DEFAULT_STORAGE_TYPE)==0)
            printf(" (default)\n");
        else
            printf("\n");
    }
    printf("   -t <options>    Storage options (default %s)\n", DEFAULT_STORAGE_OPTIONS);
    printf("   -n              Create a new store / replace old (default no)\n");
    printf("   -v              Enable verbose mode\n");
    printf("   -q              Enable quiet mode\n");
    exit(1);
}


int main(int argc, char *argv[])
{
    http_server_t *server = NULL;
    librdf_hash *storage_options = NULL;
    char* port = DEFAULT_PORT;
    const char* storage_options_str = DEFAULT_STORAGE_OPTIONS;
    int storage_new = 0;
    int opt = -1;
    
    // Make STDOUT unbuffered - we use it for logging
    setbuf(stdout, NULL);

    // Initialise Redland
    world=librdf_new_world();
    if (!world) {
        redstore_fatal("Failed to initialise librdf world");
        return -1;
    }
    librdf_world_open(world);
    librdf_world_set_logger(world, NULL, redland_log_handler);

    // Parse Switches
    while ((opt = getopt(argc, argv, "p:s:t:nvqh")) != -1) {
        switch (opt) {
            case 'p':  port = optarg; break;
            case 's':  storage_type = optarg; break;
            case 't':  storage_options_str = optarg; break;
            case 'n':  storage_new = 1; break;
            case 'v':  verbose = 1; break;
            case 'q':  quiet = 1; break;
            default:  usage(); break;
        }
    }

    // Check remaining arguments
    argc -= optind;
    argv += optind;
    if (argc==1) {
        storage_name = argv[0];
    } else if (argc>1) {
        usage();
    }
    
    // Validate parameters
    if (quiet && verbose) {
        redstore_error("Can't be quiet and verbose at the same time.");
        usage();
    }

    // Setup signal handlers
    signal(SIGTERM, termination_handler);
    signal(SIGINT, termination_handler);
    signal(SIGHUP, termination_handler);

    // Create HTTP server
    server = http_server_new();
    if (!server) {
        fprintf(stderr, "Failed to initialise HTTP server.\n");
        exit(EXIT_FAILURE);
    }

    // Configure routing
    http_server_add_handler(server, NULL, NULL, request_counter, &request_count);
    http_server_add_handler(server, "GET", "/sparql", handle_sparql_query, NULL);
    http_server_add_handler(server, "GET", "/sparql/", handle_sparql_query, NULL);
    http_server_add_handler(server, "HEAD", "/data/*", handle_graph_head, NULL);
    http_server_add_handler(server, "GET", "/data/*", handle_graph_get, NULL);
    http_server_add_handler(server, "PUT", "/data/*", handle_graph_put, NULL);
    http_server_add_handler(server, "DELETE", "/data/*", handle_graph_delete, NULL);
    http_server_add_handler(server, "GET", "/data", handle_graph_index, NULL);
    http_server_add_handler(server, "GET", "/load", handle_load_get, NULL);
    http_server_add_handler(server, "POST", "/load", handle_load_post, NULL);
    http_server_add_handler(server, "GET", "/", handle_page_home, NULL);
    http_server_add_handler(server, "GET", "/query", handle_page_query, NULL);
    http_server_add_handler(server, "GET", "/info", handle_page_info, NULL);
    http_server_add_handler(server, "GET", "/formats", handle_page_formats, NULL);
    http_server_add_handler(server, "GET", "/favicon.ico", handle_image_favicon, NULL);
    //http_server_add_handler(server, "GET", NULL, handle_remove_trailing_slash, NULL);
    
    // Set the server signature
    // FIXME: add Redland libraries to this?
    http_server_set_signature(server, PACKAGE_NAME "/" PACKAGE_VERSION);


    // Setup Redland storage
    redstore_info("Storage type: %s", storage_type);
    redstore_info("Storage name: %s", storage_name);
    storage_options = librdf_new_hash_from_string(world, NULL, storage_options_str);
    librdf_hash_put_strings(storage_options, "contexts", "yes");
    librdf_hash_put_strings(storage_options, "write", "yes");
    if (storage_new) librdf_hash_put_strings(storage_options, "new", "yes");
    // FIXME: display all storage options
    // This doesn't print properly: librdf_hash_print(storage_options, stdout);
    storage = librdf_new_storage_with_options(world, storage_type, storage_name, storage_options);
    if (!storage) {
        redstore_fatal("Failed to open %s storage '%s'", storage_type, storage_name);
        return -1;
    }

    // Create model object    
    model=librdf_new_model(world, storage, NULL);
    if(!model) {
        redstore_fatal("Failed to create model for storage.");
        return -1;
    }


    // Start listening for connections
    redstore_info("Starting HTTP server on port %s", port);
    if (http_server_listen(server, NULL, port, PF_UNSPEC)) {
        fprintf(stderr, "Failed to create HTTP socket.\n");
        exit(EXIT_FAILURE);
    }

    while(running) {
        http_server_run(server);
    }
    
    librdf_free_storage(storage);
    librdf_free_hash(storage_options);
    librdf_free_world(world);

    http_server_free(server);
 
    return 0;   // FIXME: should return non-zero if there was a fatal error
}
