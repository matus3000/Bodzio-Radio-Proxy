#include "multicast.h"
#include "ifi.h"
#include <arpa/inet.h>
#include <stdlib.h>

int mcast_join(int sockfd, const char *multicast) {
    int result = 0;
    struct ifi_info *ifi;
    struct ip_mreq ip_req;
    struct in_addr multicast_addr;

    if (inet_aton(multicast, &multicast_addr) == 0) return WRONG_ADDRESS;

    if ((ifi = get_ifi_info()) == NULL) {
        ip_req.imr_multiaddr.s_addr = multicast_addr.s_addr;
        ip_req.imr_interface.s_addr = htonl(INADDR_ANY);
        result = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ip_req, sizeof(ip_req));

    } else {
        for (struct ifi_info *tmp = ifi; tmp != NULL && result == 0; tmp = tmp->ifi_next) {
            if (tmp->ifi_flags & IFF_MULTICAST) {
                ip_req.imr_multiaddr.s_addr = multicast_addr.s_addr;
                ip_req.imr_interface.s_addr = ((struct sockaddr_in*) tmp->ifi_addr)->sin_addr.s_addr;
                result = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ip_req, sizeof(ip_req));
            }
        }
        free_ifi_info(ifi);
    }

    return result;
}

int mcast_set_ttl(int sockfd, int _ttl) {
    u_char ttl = (u_char) _ttl;
    return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(u_char));
}
