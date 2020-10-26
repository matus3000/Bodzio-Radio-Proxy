#ifndef __UDP_MESSAGE__
#define __UDP_MESSAGE__
#include <inttypes.h>
#include <stdbool.h>

/**
 * Trochę redundantna struktura, bo to tak naprawdę napis.
 */
struct udp_message {
    char *buff;
    uint16_t len;
    uint16_t size;
};

/**
 * @brief Struktura na tworzenie wiadomości udp.
 */
struct udp_message *udp_message_new(uint16_t size);

/**
 * @brief Usuń strukturę
 * @param[in] message Struktura do usunięcia, może być NULLEM.
 */
void udp_message_delete(struct udp_message *message);

/**
 * @brief Wczytaj maksymalnie len bajtów od pozycji start z deskryptora fd
 */
int udp_message_read(struct udp_message *msg, uint16_t start, int fd, uint16_t len);

/**
 * Skopiuj zawartośc bufora buff do wiadomości zapisując pierwszy bajt w miejscu start
 */
int udp_message_strncmp(struct udp_message *msg, uint16_t start, const char *buff, uint16_t n);

/**
 * @brief Zwróć długość wiadomości
 */
uint16_t udp_message_get_len(struct udp_message *msg);

/**
 * @brief Wyczyść wiadomość.
 */
void udp_message_clear(struct udp_message *msg);

#endif
