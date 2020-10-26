#ifndef __HTTP_REQUEST__
#define __HTTP_REQUEST__
#include "objective_c/my_string.h"
#include "http.h"

struct shout_request_message {
    char *request_target;
    char *uri_host;
    /* enum header_connection_option connection; */
    bool metadata;
};

int print_shoutcast_request_message(struct resizable_string *str,
                               struct shout_request_message *rq);

#endif
