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
    } else if (strcmp(request.url, "/favicon.ico")==0) {
        return handle_favicon(&request);
    } else {
        return handle_error(&request, MHD_HTTP_NOT_FOUND);
    }
}

// Display how to use this program
static void usage()
{
    printf("%s version %s\n\n", PACKAGE_NAME, PACKAGE_VERSION);
    printf("Usage: %s [options]\n", PACKAGE_TARNAME);
    printf("   -p <port>     Port number to run HTTP server on (default %d)\n", DEFAULT_PORT);
    printf("   -v            Enable verbose mode\n");
    printf("   -q            Enable quiet mode\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    struct MHD_Daemon *daemon = NULL;
    int port = DEFAULT_PORT;
    int opt = -1;
    
    // Make STDOUT unbuffered - we use it for logging
    setbuf(stdout, NULL);

    // Parse Switches
    while ((opt = getopt(argc, argv, "p:vqh")) != -1) {
        switch (opt) {
            case 'p':  port = atoi(optarg); break;
            case 'v':  verbose = 1; break;
            case 'q':  quiet = 1; break;
            default:  usage(); break;
        }
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
 
    return 0;   // FIXME: should return non-zero if there was a fatal error
}
