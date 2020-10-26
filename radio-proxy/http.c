#include "http.h"
#include "objective_c/my_string.h"
#include "net_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

bool is_cookie_octet(char c) {
    return c == 0x21 || (c >= 0x23 && c <= 0x2b) ||
        (c >= 0x3c && c <= 0x5b) || (c >= 0x5d && c <=0x7e);
}

bool is_tchar(char c) {
    return isalnum(c) || c == '!' || c ==  '#' || c ==  '$' || c == '%' ||
        c ==  '&' || c == '\'' || c == '*' || c == '+' || c ==  '!' ||
        c == '-' || c == '.' ||  c == '^' || c == '_' || c == '`' || c == '|'
        || c == '~';
}

bool is_vchar(char c) {
    return c > 0x20 && c < 0x7f;
}

char *get_header_field_name(enum header_field field_name) {
    static char * host_str = "HOST";
    static char * connection_str = "CONNECTION";
    static char * icy_metadata_str = "ICY-METADATA";
    static char * icy_metaint_str = "ICY-METATINT";
    static char * icy_name_str = "ICY-NAME";

    switch (field_name) {
    case HOST: return host_str;
    case CONNECTION: return connection_str;
    case ICY_METADATA: return icy_metadata_str;
    case ICY_NAME: return icy_name_str;
    case ICY_METAINT: return icy_metaint_str;
    default: return NULL;
    }
}
