#ifndef __UDP_SOCKETS__
#define __UDP_SOCKETS__
#include <inttypes.h>
#include <netinet/in.h>

enum udp_socket_type {
    T_UNICAST = 0x1
};

struct udp_sockets {
    int unicastfd;///< Gniazdo UDP
    char _[12];///< Puste pola, aby można było korzystać z malloca.
};

/**
 * @brief Funkcja inicjuje strutkurę udp_sockets. Otwiera gniazdo UDP
 *        nasłuchujące na porcie @p port i adresie INADDR_ANY oraz na adresie rozgłoszeniowym.
 *        Jeśli agrgument @p multicast jest wskaźnikiem niepustym, to także gniazdo jest dodane do
 *        grupy rozgłoszeniwej.
 *
 * @return 0 jeśli udało się zainicjować strukturę, liczba różna od 0 jeśli wystąpił błąd lub
 *         adres wskazywany przez @p multicast nie jest adresem rozsyłania grupowego albo nie udało
 *         się dołączyć do grupy.
 */
int udp_sockets_init(struct udp_sockets *udp_sockets, uint16_t port, char *multicast);

/**
 * @brief Funkcja napierw alokuje pamięć na strukturę udp_sockets a potem wywołuje na niej
 *        udp_sockets_init.
 * @return Niepusty wskaźnik, jeśli udało się zaalokować pamięć i wywołanie udp_sockets_init
 *         przekazało 0; wpp. NULL.
 */
struct udp_sockets *udp_sockets_new(uint16_t port, char *multicast);

/**
 * @brief Usuwa strukturę, zamyka deskryptrory, ale nie zwalnia pamięci zaalokowanej na
 *        @p udp_sockets.
 */
void udp_sockets_destroy(struct udp_sockets *udp_sockets);

/**
 * @brief Usuwa sturkturę i zwalnia pamięć zaalokowaną na udp_sockets.
 */
void udp_sockets_delete(struct udp_sockets *udp_sockets);

/**
 * @brief Funkcja zwraca największy numer deskryptora plus 1 dla funkcji select.
 */
int udp_sockets_maxfdp1(struct udp_sockets *udp_sockets, int maxfdp1);

/**
 * @brief Przekazuje deskryptor gniazda danego typu.
 */
int udp_sockets_get(struct udp_sockets *udp_sockets, enum udp_socket_type type);

/**
 * @brief Dodaje gniazda znajdujące się w strukturze udp_sockets do zbioru @p set.
 */
void udp_sockets_add_to_fds(struct udp_sockets *udp_sockets, fd_set *set);
#endif
