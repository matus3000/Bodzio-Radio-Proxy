#ifndef __PROXY_CLIENT__
#define __PROXY_CLIENT__
#include "shout_response.h"
#include "udp_sockets.h"


/**
 * @brief Funkcja pełni funkcję proxy.
 */
int run_proxy_loop(struct udp_sockets *udp_sockets, struct HTTP_response_message *response,
                   int sigfd, int readfd, int timeout);

#endif
