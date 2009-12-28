#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#include "redstore.h"


// ------- Globals -------
int quiet = 0;					// Only display error messages
int verbose = 0;				// Increase number of logging messages
int running = 1;        // True while still running
librdf_world* world = NULL;
librdf_storage* storage = NULL;


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

int redland_log_handler(void* user, librdf_log_message* msg)
{
    int level = librdf_log_message_level(msg);
    int code = librdf_log_message_code(msg);
    const char* message = librdf_log_message_message(msg);
    printf("redland_log_handler: code=%d level=%d message=%s\n", code, level, message);
    return 0;
}

static int dispatcher (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
    http_request_t request;
    request.cls = cls;
    request.connection = connection;
    request.url = url;
    request.method = method;
    request.version = version;
    request.upload_data = upload_data;
    request.upload_data_size = upload_data_size;
    request.con_cls = con_cls;
    
    redstore_debug("%s: %s", method, url);

    if (strcmp(request.url, "/")==0) {
        return handle_homepage(&request);
    } else if (strcmp(request.url, "/query")==0) {
        return handle_querypage(&request);
    } else if (strcmp(request.url, "/graphs")==0) {
        return handle_graph_index(&request);
    } else if (strcmp(request.url, "/favicon.ico")==0) {
        return handle_favicon(&request);
    } else {
        return handle_error(&request, MHD_HTTP_NOT_FOUND);
    }
}

// Display how to use this program
static void usage()
{
    int i;
    printf("%s version %s\n\n", PACKAGE_NAME, PACKAGE_VERSION);
    printf("Usage: %s [options] [<name>]\n", PACKAGE_TARNAME);
    printf("   -p <port>       Port number to run HTTP server on (default %d)\n", DEFAULT_PORT);
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


/** data type used to describe hash key and data */
struct librdf_hash_datum_s
{
  librdf_world *world;
  void *data;
  size_t size;
  /* used internally to build lists of these  */
  struct librdf_hash_datum_s *next;
};
typedef struct librdf_hash_datum_s librdf_hash_datum;

librdf_hash_datum* librdf_new_hash_datum(librdf_world *world, void *data, size_t size);
void librdf_free_hash_datum(librdf_hash_datum *ptr);


int main(int argc, char *argv[])
{
    struct MHD_Daemon *daemon = NULL;
    librdf_hash *storage_options = NULL;
    librdf_model *model = NULL;
    int port = DEFAULT_PORT;
    const char* storage_name = DEFAULT_STORAGE_NAME;
    const char* storage_type = DEFAULT_STORAGE_TYPE;
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
            case 'p':  port = atoi(optarg); break;
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

    // Start HTTP server
    redstore_info("Starting HTTP server on port %d", port);
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, port,
                   NULL, NULL,
                   &dispatcher, NULL,
                   MHD_OPTION_END
    );
    if (daemon == NULL) {
        redstore_fatal("Failed to start HTTP daemon: \n");
    }
    
    while (running) {
        sleep(1);
    }
    
    MHD_stop_daemon(daemon);
    
    librdf_free_storage(storage);
    librdf_free_hash(storage_options);
    librdf_free_world(world);
 
    return 0;   // FIXME: should return non-zero if there was a fatal error
}
