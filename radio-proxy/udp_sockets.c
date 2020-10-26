#include "udp_sockets.h"
#include "multicast.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#define BROADCAST_FLAG 1
#define REUSEADDR_FLAG 2
#define SAFE_CLOSE(x) if (x > 0) close(x);
#define SAFE_FD_SET(x, fdsetp) if (x >= 0) FD_SET(x, fdsetp);


static inline int64_t max(int64_t x, int64_t y) {
    return (x > y) ? x : y;
}


static void sockaddr_in_init(struct sockaddr_in *addr, uint16_t port_num) {
    bzero(addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port_num);
}

/**
 * @brief Utwórz gniazdo, wykonaj bind i operacje wskazane przez flagi.
 */
static int udp_socket_bind(int flags, struct sockaddr_in *sockname) {
    int fd = -1;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return -1;

    if (flags & BROADCAST_FLAG) {
        int optval = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) != 0) {
            goto error;
        }
    }
    if (fd > 0) {
        if (bind(fd, (struct sockaddr*) sockname, sizeof(struct sockaddr_in)) != 0 &&
            !(flags & REUSEADDR_FLAG))
        {
            goto error;
        }
    }

    return fd;
error:
    if (fd > 0) close(fd);
    return -1;
}

struct udp_sockets *udp_sockets_new(uint16_t port, char *multicast) {
    struct udp_sockets *result = malloc(sizeof(struct udp_sockets));

    if (result && udp_sockets_init(result, port, multicast) != 0) {
        free(result);
        result = NULL;
    }

    return result;
}

int udp_sockets_init(struct udp_sockets *udp_sockets, uint16_t port_num, char *multicast) {
    bzero(udp_sockets, sizeof(struct udp_sockets));

    struct sockaddr_in addr;
    sockaddr_in_init(&addr, port_num);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_sockets->unicastfd = udp_socket_bind(BROADCAST_FLAG, &addr);

    if (udp_sockets->unicastfd < 0) {
        udp_sockets_destroy(udp_sockets);
        return -1;
    }
    if (multicast && mcast_join(udp_sockets->unicastfd, multicast) != 0) {
        udp_sockets_destroy(udp_sockets);
        return -1;
    }
    mcast_set_ttl(udp_sockets->unicastfd, MCAST_SITE_LOCAL);

    return 0;
}

void udp_sockets_destroy(struct udp_sockets *udp_sockets) {
    if (!udp_sockets) return;

    SAFE_CLOSE(udp_sockets->unicastfd);
}

void udp_sockets_delete(struct udp_sockets *udp_sockets) {
    if (!udp_sockets) return;

    udp_sockets_destroy(udp_sockets);
    free(udp_sockets);
}

int udp_sockets_get(struct udp_sockets *udp_sockets, enum udp_socket_type type) {
    if (type == T_UNICAST) return udp_sockets->unicastfd;
    else return -1;
}

void udp_sockets_add_to_fds(struct udp_sockets *udp_sockets, fd_set *set) {
    SAFE_FD_SET(udp_sockets->unicastfd, set);
}

int udp_sockets_maxfdp1(struct udp_sockets *udp_sockets, int maxfdp1) {
    return max(maxfdp1, udp_sockets->unicastfd + 1);
}
