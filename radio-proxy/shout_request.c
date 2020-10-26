#include "shout_request.h"
#include "my_error.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

const char *getCRLF() {
    static char *result = "\r\n";
    return result;
}

static inline int print_CRLF(struct resizable_string *str) {
    return resizable_string_append_str(str, (char *) getCRLF());
}

static inline int print_colon(struct resizable_string *str) {
    return resizable_string_append_c(str, ':');
}

static inline int print_SP(struct resizable_string *str) {
    return resizable_string_append_c(str, ' ');
}

static char *get_request_token(enum request_type token) {
    static char *get_token = "GET";
    switch (token) {
    case GET: return get_token;
    default: return NULL;
    }
}

static inline int print_request_token(struct resizable_string *str) {
    return resizable_string_append_str(str,
                                       get_request_token(GET));
}

static inline int print_request_target(struct resizable_string *str,
                                       struct shout_request_message *rq) {
    return resizable_string_append_str(str, rq->request_target);
}

char *get_HTTP_version(enum HTTP_version version){
    static char *http_1_1 = "HTTP/1.1";
    static char *http_1_0 = "HTTP/1.0";
    switch (version) {
    case HTTP1_0: return http_1_0;
    case HTTP1_1: return http_1_1;
    default: return NULL;
    }
}

static inline int print_HTTP_version(struct resizable_string *str) {
    return resizable_string_append_str(str, get_HTTP_version(HTTP1_0));
}

int print_request_line(struct resizable_string *str, struct shout_request_message *rq) {
    int result = 0;

    if ((result = print_request_token(str)) != 0) return result;
    if ((result = print_SP(str)) != 0) return result;
    if ((result = print_request_target(str, rq)) != 0) return result;
    if ((result = print_SP(str)) != 0) return result;
    if ((result = print_HTTP_version(str)) != 0) return result;
    result = print_CRLF(str);

    return result;
}

static inline int print_header_field_name(struct resizable_string *str,
                                          enum header_field field_name) {
    return resizable_string_append_str(str, get_header_field_name(field_name));
}

static int print_host_field_value(struct resizable_string *str, struct shout_request_message *rq) {
    int result = 0;

    result |= resizable_string_append_str(str, rq->uri_host);

    return result;
}

/* static char *get_connection_field_value(enum header_connection_option op) { */
/*     static char * close_str = "CLOSE"; */
/*     switch (op) { */
/*     case close_connection: return close_str; */
/*     default: return NULL; */
/*     } */
/* } */

/* static int print_connection_field_value(struct resizable_string *str, */
/*                                         struct shout_request_message *rq) { */
/*     /\* char * tmp = get_connection_field_value(rq->connection); *\/ */

/*     if (tmp) { */
/*         return resizable_string_append_str(str, tmp); */
/*     } else { */
/*         return -1; */
/*     } */
/* } */

static int print_icy_metadata_field_value(struct resizable_string *str,
                                           struct shout_request_message *rq) {
    return (rq->metadata) ? resizable_string_append_c(str, '1') :
        resizable_string_append_c(str, '0');
}

int print_header_field_value(struct resizable_string *str, enum header_field field_name,
                             struct shout_request_message *rq) {
    switch (field_name) {
    case HOST: return print_host_field_value(str, rq);
    /* case CONNECTION: return print_connection_field_value(str, rq); */
    case ICY_METADATA: return print_icy_metadata_field_value(str, rq);
    default: return REQUEST_UNIMPLEMENTED_HF;
    }
}

int print_header_field(struct resizable_string *str, enum header_field field_name,
                       struct shout_request_message *rq) {
    int result = 0;

    if ((result = print_header_field_name(str, field_name))) return result;
    if ((result = print_colon(str))) return result;
    if ((result = print_SP(str))) return result;
    if ((result = print_header_field_value(str, field_name, rq))) return result;
    result = print_CRLF(str);

    return result;
}

int print_shoutcast_request_message(struct resizable_string *str, struct shout_request_message *rq) {
    if (!str) return -1;
    int result = 0;
    resizable_string_clear(str);

    if ((result = print_request_line(str, rq))) return result;
    if ((result = print_header_field(str, ICY_METADATA, rq))) return result;
    /* if ((result = print_header_field(str, CONNECTION, rq))) return result; */
    result = print_CRLF(str);

    return result;
}
