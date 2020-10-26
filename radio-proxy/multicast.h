/**
 * @author W. Stevens "Unix programowanie usług sieciowych".
 */
#ifndef __MULTICAST__
#define __MULTICAST__
#include <netinet/in.h>
#define WRONG_ADDRESS -2

#define MCAST_NODE_LOCAL 0
#define MCAST_LINK_LOCAL 1
#define MCAST_SITE_LOCAL 5

/**
 * @brief Funkcja podłącza gniazdo sockfd do grupy, której adres wskazuje napis @p multicast
 *        na wszystkich interfejsach obsługujących multicast, jeśli nie uda się pobrać informacji
 *        o interfejsach to adresem interfejsu jest INADDR_ANY.
 */
int mcast_join(int sockfd, const char *multicast);

/**
 * @brief Ustaw wartość TTL.
 */
int mcast_set_ttl(int sockfd, int _ttl);

#endif
