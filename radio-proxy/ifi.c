#include "ifi.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>

int get_siocgifconf_buff_len(int sockfd) {
    int result = 0;
    struct ifconf ifc;
    ifc.ifc_ifcu.ifcu_buf = NULL;
    ifc.ifc_len = 0;

    if (ioctl(sockfd, SIOCGIFCONF, &ifc) != 0) {
        result = -1;
    } else {
        if (ifc.ifc_len <= 0) {
            result = -1;
        } else {
            result = ifc.ifc_len;
        }
    }

    return result;
}

static int init_ifconf(struct ifconf *ifc, int sockfd) {
    int len = get_siocgifconf_buff_len(sockfd);
    if (len < 0) return -1;
    char *buff = malloc(len);
    ifc->ifc_len = len;
    ifc->ifc_ifcu.ifcu_buf = buff;
    if (ioctl(sockfd, SIOCGIFCONF, ifc) != 0) {
        free(buff);
        bzero(ifc, sizeof(struct ifconf));
        return -1;
    } else {
        return 0;
    }
}

static void destroy_ifconf(struct ifconf *ifc) {
    if (ifc->ifc_ifcu.ifcu_buf != NULL) {
        free(ifc->ifc_ifcu.ifcu_buf);
    }
    bzero(ifc, sizeof(struct ifconf));
}

static int is_alias(char *name) {
    char *ptr = strchr(name, ':');

    return ptr != NULL;
}


static int get_flags(const char *_ifname, int sockfd, int *flags) {
    int result = 0;
    struct ifreq ifreq;
    strncpy(ifreq.ifr_name, _ifname, IFNAMSIZ);
    ifreq.ifr_name[IFNAMSIZ - 1] = 0;

    if (ioctl(sockfd, SIOCGIFFLAGS, &ifreq) != 0) {
        result = -1;
    } else {
        *flags = ifreq.ifr_flags;
    }

    return result;
}

struct ifi_info *get_ifi_info() {
    struct ifi_info *ifi, *ifihead, **ifipnext;

    int sockfd, flags;
    struct ifconf ifc;
    struct ifreq *ifr;
    struct sockaddr_in *sinptr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return NULL;
    if (init_ifconf(&ifc, sockfd) != 0) return NULL;

    ifihead = NULL;
    ifipnext = &ifihead;

    for (unsigned long i = 0; i < (ifc.ifc_len / sizeof(struct ifreq)); ++i) {
        ifr = &ifc.ifc_ifcu.ifcu_req[i];
        if (ifr->ifr_addr.sa_family != AF_INET || is_alias(ifr->ifr_name)) continue;

        if (get_flags(ifr->ifr_name, sockfd, &flags) == 0 && (flags & IFF_UP)) {
            ifi = calloc(1, sizeof(struct ifi_info));
            sinptr = calloc(1, sizeof(struct sockaddr_in));
            if (ifi && sinptr) {
                ifi->ifi_addr = (struct sockaddr*) sinptr;
                memcpy(ifi->ifi_addr, &ifr->ifr_addr, sizeof(struct sockaddr_in));
                ifi->ifi_flags = flags;
                memcpy(ifi->ifi_name, ifr->ifr_name, IFI_NAME);
                ifi->ifi_name[IFI_NAME - 1] = 0;
                *ifipnext = ifi;
                ifipnext = &ifi->ifi_next;
            } else {
                free(ifi);
                free(sinptr);
            }
        }
    }
    *ifipnext = NULL;

    destroy_ifconf(&ifc);

    return ifihead;
}

void free_ifi_info(struct ifi_info *ifi) {
    struct ifi_info *tmp;
    while (ifi != NULL) {
        tmp = ifi->ifi_next;
        free(ifi->ifi_addr);
        free(ifi);
        ifi = tmp;
    }
}
