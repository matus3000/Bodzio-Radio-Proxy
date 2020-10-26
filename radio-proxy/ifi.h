/**
 * @author W. Stevens, dostosowanie do własnych potrzeb Mateusz Bodziony.
 */
#ifndef __IFI__
#define __IFI__

#include <net/if.h>
#define IFI_NAME 16

struct ifi_info {
    char ifi_name[IFI_NAME]; ///< Nazwa intefejsu zakończona znakiem pustym,
    short ifi_flags;///< Stałe IFF_xx zdefiniowane w IFI_xxx
    struct sockaddr *ifi_addr;///< Adres podstawowy
    struct ifi_info *ifi_next;///< Następna struktura
};

/**
 * @brief Pobierz informację o aktywnych interfejsach obsługującyh IPv4.
 */
struct ifi_info *get_ifi_info();

/**
 * @brief Zwolnij pamięć na strukturę ifi_info.
 */
void free_ifi_info(struct ifi_info *);

#endif
