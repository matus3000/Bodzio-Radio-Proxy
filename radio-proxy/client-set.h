#ifndef __CLIENT_SET__
#define __CLIENT_SET__
#include <netinet/in.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum client_set_info {
    CS_CONNECTED_SET, CS_WAITING_SET
};

struct client_set;

struct client_set *client_set_new(unsigned int timeout);

/**
 * @param[in] client_set Struktura do usunięcia może, być NULLEM.
 */
void client_set_delete(struct client_set *client_set);

/**
 * @brief Funkcja przekazuje odpowiedź na pytanie wyznaczone przez parametr flaga.
 * @param[in] info_type Flaga CS_WAITING_SET lub CS_CONNECTED_SET
 * @return True jeśli odpowiedź na pytanie jest pozytywna false wpp.
 */
bool client_set_contains(struct client_set *client_set, struct sockaddr_in *addr,
                               enum client_set_info set_type);

int client_set_refresh_client(struct client_set *client_set, struct sockaddr_in *addr,
        enum client_set_info set_type);

/**
 * @brief Dodaje klienta do zbioru klientów oczekujących.
 *        Jeżeli klient oczekuje nic nie robi.
 */
int client_set_add_client(struct client_set *client_set, struct sockaddr_in *addr);

/**
 * @brief Wysyła datagram do wszystkich połączonych klientów.
 */
int client_set_send_datagram(struct client_set *client_set, int udpsocket, char *buff, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
