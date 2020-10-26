#include "udp_message.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

struct udp_message *udp_message_new(uint16_t size) {
    struct udp_message *result = malloc(sizeof(struct udp_message));

    if (result != NULL) {
        result->buff = malloc((size > 16) ? size : 16);
        if (result->buff) {
            result->size = (size > 16) ? size : 16;
            result->len = 0;
        } else {
            free(result);
            result = NULL;
        }
    }
    return result;
}

void udp_message_delete(struct udp_message *message) {
    if (!message) return;

    if (message->buff) free(message->buff);
    free(message);
}

int udp_message_read(struct udp_message *msg, uint16_t start, int fd, uint16_t len) {
    if (start >= msg->size) return -1;
    if (start + len > msg->size) return -1;

    int readed = 0;
    len = (len + start >= msg->size) ? msg->size - start : len;
    readed = read(fd, &msg->buff[start], len);
    msg->len = (readed > 0 && start + readed > msg->len) ? start + readed : msg->len;

    return readed;
}

/**
 * Skopiuj zawartośc bufora buff do wiadomości zapisując pierwszy bajt w miejscu start
 */
int udp_message_strncmp(struct udp_message *msg, uint16_t start, const char *buff, uint16_t n) {
    if (start >= msg->size) return -1;
    if (start + n > msg->size) return -1;

    memcpy(&msg->buff[start], buff, n);
    if (msg->len < start + n) msg->len = start + n;

    return 0;
}

/**
 * @brief Zwróć długość wiadomości
 */
uint16_t udp_message_get_len(struct udp_message *msg) {
    return msg->len;
}

/**
 * @brief Wyczyść wiadomość.
 */
void udp_message_clear(struct udp_message *msg) {
    msg->len = 0;
}
