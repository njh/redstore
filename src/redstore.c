#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "config.h"
#include "redstore.h"


#define PORT 8080

int keep_running = 1;


int dispatcher (void *cls, struct MHD_Connection *connection,
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
    
    printf("%s: %s\n", method, url);

    if (strcmp(request.url, "/")==0) {
        return handle_homepage(&request);
    } else if (strcmp(request.url, "/favicon.ico")==0) {
        return handle_favicon(&request);
    } else {
        return handle_error(&request, MHD_HTTP_NOT_FOUND);
    }
}

int main()
{
    struct MHD_Daemon *daemon;
    
    daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                   &dispatcher, NULL, MHD_OPTION_END);
    if (daemon == NULL) {
        fprintf(stderr, "Failed to start daemon.\n");
        return 1;
    }
    
    while (keep_running) {
        sleep(1);
    }
    
    MHD_stop_daemon (daemon);
    return 0;
}
