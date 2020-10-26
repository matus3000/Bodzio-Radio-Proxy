#ifndef __HTTP__
#define __HTTP__
#include <stdbool.h>
#include <inttypes.h>

enum request_type{
    def = 0, GET
};

enum header_field {
    WRONG = -1,
    HOST,
    ICY_METADATA,
    CONNECTION,
    ICY_METAINT,
    ICY_NAME,
    UNKNOWN
};

enum header_connection_option {
    close_connection,
};

enum HTTP_version {
    HTTP1_0, HTTP1_1
};

bool is_cookie_octet(char c);
bool is_vchar(char c);
bool is_tchar(char c);
char *get_header_field_name(enum header_field field_name);

#endif
