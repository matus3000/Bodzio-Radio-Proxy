#include "rp_command_line.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#define EMPTY_ARG -1
#define OVERFLOW 1
#define MALFORMED -2
#define LONG_MAX 0x7fffffff

static uint32_t string_to_uint32_t(char *string, int *err) {
    uint64_t result = 0;
    char *endptr;
    *err = 0;
    while (isspace(*string)) ++string;

    if (isdigit(*string) || *string == '+') {
        result = strtoull(string, &endptr, 10);

        if (string == endptr) {
            *err = -1;
        } else if (result > 0xffffffff) {
            *err = OVERFLOW;
            result = 0xffffffff;
        } else {
            while (isspace(*endptr)) ++endptr;
            if (*endptr != '\0') *err = MALFORMED;
            else *err = 0;
        }
    } else {
        *err = MALFORMED;
    }

    return result;
}

int parse_timeout(char *timeout, long *tv_sec) {
    int err = 0;
    int result = 0;
    uint32_t res = string_to_uint32_t(timeout, &err);

    if (err == 0 && res <= LONG_MAX) {
        *tv_sec = (long) res;
    } else {
        result = (err != 0) ? err : OVERFLOW;
    }

    return result;
}


int get_parameter_string(int argc, char **argv, char parameter, char **string) {
    int result = 0;
    int i = 1;

    *string = NULL;
    while (i < argc
           && !(argv[i][0] == '-' && argv[i][1] == parameter && argv[i][2] == 0)) ++i;

    if (i < argc - 1 && i % 2 == 1) *string = argv[i+1];
    else if (i < argc) result = EMPTY_ARG;

    return result;
}

int get_host(int argc, char **argv, char **destination) {
    int result = get_parameter_string(argc, argv, 'h', destination);

    if (result == 0 && !(*destination)) {
        result = EMPTY_ARG;
    }

    return result;
}

int get_resource(int argc, char **argv, char **destination) {
    int result = get_parameter_string(argc, argv, 'r', destination);

    if (result == 0 && !(*destination)) {
        result = EMPTY_ARG;
    }

    return result;
}

int get_port(int argc, char **argv, char **destination) {
    int result = get_parameter_string(argc, argv, 'p', destination);

    if (result == 0 && !(*destination)) {
        result = EMPTY_ARG;
    }

    return result;
}

int get_metadata(int argc, char **argv, bool *metadata) {
    int result = 0;
    char *metadata_string;

    if ((result = get_parameter_string(argc, argv, 'm', &metadata_string)) != 0) {
        return result;
    }

    if (metadata_string) {
        if (strcmp(metadata_string, "yes") == 0) {
            *metadata = true;
        } else if (strcmp(metadata_string, "no") == 0) {
            *metadata = false;
        } else {
            result = -1;
        }
    } else {
        *metadata = false;
    }

    return result;
}


int get_timeout(int argc, char **argv, struct timeval *timeout) {
    int result = 0;
    char *timeout_string;

    if ((result = get_parameter_string(argc, argv, 't', &timeout_string)) != 0) {
        return result;
    }

    timeout->tv_usec = 0;
    if (timeout_string) {
        result = parse_timeout(timeout_string, &timeout->tv_sec);
    } else {
        timeout->tv_sec = 5;
    }

    return result;
}

int get_proxy_timeout(int argc, char **argv, long *proxy_timeout) {
    int result = 0;
    char *timeout_string;

    if ((result = get_parameter_string(argc, argv, 'T', &timeout_string)) != 0) {
        return result;
    }

    if (timeout_string) {
        result = parse_timeout(timeout_string, proxy_timeout);
    } else {
        *proxy_timeout = 5;
    }

    return result;

}

int get_proxy_port(int argc, char **argv, int *proxy_port) {
    int result = 0;
    char *port_string;

    if ((result = get_parameter_string(argc, argv, 'P', &port_string)) != 0) {
        return result;
    }

    if (port_string) {
        int error = 0;
        uint32_t port_num = string_to_uint32_t(port_string, &error);
        *proxy_port = (int) port_num;
        if (error == 0 && port_num <= 0xffff && port_num > 0) {
            result = 0;
        } else {
            result = -1;
        }
    } else {
        *proxy_port = -1;
    }

    return result;
}

int get_proxy_group(int argc, char **argv, char **proxy_group) {
    return get_parameter_string(argc, argv, 'B', proxy_group);
}

int parse_command_line_args(int argc, char **argv, struct command_line_args *result) {
    if (get_host(argc, argv, &result->host) != 0) return -1;
    if (get_resource(argc, argv, &result->resource) != 0) return -1;
    if (get_port(argc, argv, &result->port) != 0) return -1;
    if (get_metadata(argc, argv, &result->metadata) != 0) return -1;
    if (get_timeout(argc, argv, &result->timeout) != 0) return -1;
    if (get_proxy_port(argc, argv, &result->proxy_port) != 0) return -1;
    if (get_proxy_group(argc, argv, &result->proxy_group) != 0) return -1;
    if (get_proxy_timeout(argc, argv, &result->proxy_timeout) != 0) return -1;

    if (result->proxy_port < 0) {
        char *timeout_string;
        get_parameter_string(argc, argv, 'T', &timeout_string);

        if (timeout_string || result->proxy_group) return -1;
    }
    return 0;
}

