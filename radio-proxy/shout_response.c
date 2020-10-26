#include "shout_response.h"
#include "my_error.h"
#include <string.h>
#include <stdlib.h>

static int enum_to_index(enum header_field field_type) {
    switch(field_type) {
    case ICY_METAINT: return 0;
    case ICY_NAME: return 1;
    default: return -1;
    }
}

int HTTP_response_init(struct HTTP_response_message *response) {
    bzero(response, sizeof(struct HTTP_response_message));
    return 0;
}

void HTTP_response_destroy(struct HTTP_response_message *response) {
    if (!response) return;
    
    struct header_field_structure *header = HTTP_response_get_header(response, ICY_NAME);
    if (header->was_set) {
        free(header->value.as_ptr);
    }
}

int HTTP_response_set_status_line(struct HTTP_response_message *response, char *status_line) {
    response->status_line = status_line;
    return 0;
}

static char * copy_string(const char *str) {
    size_t len = strlen(str);
    len = (len < 15) ? 15 : len;
    char *name = malloc(len + 1);

    if (name) {
        strcpy(name, str);
    }
    return name;
}

int HTTP_response_set_icy_name(struct HTTP_response_message *response, const char *icy_name) {
    struct header_field_structure *header = HTTP_response_get_header(response, ICY_NAME);

    if (header->was_set) return ALREADY_SET;

    char *name = copy_string(icy_name);
    if (name) {
        header->was_set = true;
        header->value.as_ptr = name;
        return 0;
    } else {
        return ERRNO_ERROR;
    }
}

int HTTP_response_set_icy_metaint(struct HTTP_response_message *response,
                                  size_t icy_metaint) {
    struct header_field_structure *header = HTTP_response_get_header(response, ICY_METAINT);

    if (header->was_set) {
        return ALREADY_SET;
    } else {
        header->value.as_u64 = icy_metaint;
        header->was_set = true;
        return 0;
    }
}

const char * HTTP_response_get_status_line(struct HTTP_response_message *response) {
    return response->status_line;
}

const char* HTTP_response_get_icy_name(struct HTTP_response_message *response) {
    struct header_field_structure *icy_name_header = HTTP_response_get_header(response, ICY_NAME);

    if (icy_name_header->was_set) {
        return icy_name_header->value.as_ptr;
    } else {
        return NULL;
    }
}

int HTTP_response_get_icy_metaint(struct HTTP_response_message *response,
                                  size_t *icy_metaint) {
    struct header_field_structure *header = HTTP_response_get_header(response, ICY_METAINT);

    if (header->was_set) {
        *icy_metaint = header->value.as_u64;
        return 0;
    } else {
        return NOT_SET;
    }

}


struct header_field_structure* HTTP_response_get_header(struct HTTP_response_message *response,
                                                        enum header_field field_type) {
    int index = enum_to_index(field_type);
    if (index >= 0 && index <= 1) {
        return &response->headers[index];
    } else {
        return NULL;
    }
}
