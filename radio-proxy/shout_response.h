#ifndef __HTTP_RESPONSE__
#define __HTTP_RESPONSE__
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include "net_utils.h"
#include "http.h"

struct header_field_structure {
    union _{
        int64_t as_i64;
        uint64_t as_u64;
        void *as_ptr;
    } value;
    bool was_set;
};

struct HTTP_response_message {
    char *status_line;
    struct header_field_structure headers[2];
};

int HTTP_response_init(struct HTTP_response_message *response);

void HTTP_response_destroy(struct HTTP_response_message *response);

int HTTP_response_set_icy_name(struct HTTP_response_message *response, const char *icy_name);

int HTTP_response_set_icy_metaint(struct HTTP_response_message *response,
                                     size_t icy_metaint);

int HTTP_response_set_status_line(struct HTTP_response_message *response,
                                  char *status_line);

const char * HTTP_response_get_status_line(struct HTTP_response_message *response);

const char * HTTP_response_get_icy_name(struct HTTP_response_message *response);

int HTTP_response_get_icy_metaint(struct HTTP_response_message *response,
                                     size_t *icy_metaint);

struct header_field_structure* HTTP_response_get_header(struct HTTP_response_message *response,
                                                      enum header_field field_type);

int receive_HTTP_response_message(struct my_stream *stream, struct HTTP_response_message *response);

#endif
