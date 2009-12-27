
#include "redstore.h"

int handle_homepage(http_request_t *request)
{
    const char data[] = "<p>Hello World!</p>";

    return handle_html_page(request, MHD_HTTP_OK, "Homepage", data);
}
