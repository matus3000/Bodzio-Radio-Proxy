#ifndef __RADIO_PROXY_COMMAND_LINE__
#define __RADIO_PROXY_COMMAND_LINE__
#include <sys/time.h>
#include <stdbool.h>

struct command_line_args {
    char *host;
    char *resource;
    char *port;
    bool metadata;
    struct timeval timeout;
    int proxy_port;///< Domyślna wartość mniejsza bądź równa 0.
    long proxy_timeout;///< Wartośc nieokreślona gdy proxy_port jest domyślne
    char *proxy_group;///< Wartośc nieokreślona gdy proxy_port jest domyślne
};

int parse_command_line_args(int argc, char **argv, struct command_line_args *result);

#endif
