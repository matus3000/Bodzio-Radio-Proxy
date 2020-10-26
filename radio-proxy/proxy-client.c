#include "proxy-client.h"
#include "client-set.h"
#include "udp_message.h"
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <stdlib.h>
#define MAX_UDP 65507 ///< Maksymalna długość ciała datagramu UDP, stad też maksymalna dlugość buffora w strukturze udp_message.
#define MIN_UDP 5///< Minimalna długość bufora w strukturze udp_message
#define PAYLOAD_SIZE 4080///< 255 * 16 . Maksymalna długość metadata i dla mnie maksymalny rozmiar danych w datagramie.
#define NOT_SET -123
#define READ_ERROR -1

enum type_field {
    DISCOVER = 1,
    IAM = 2,
    KEEPALIVE = 3,
    AUDIO = 4,
    METADATA = 6,
    WRONG_ANNOUNCEMENT = 7
};

/**
 * @brief Struktura opakowująca udp_message. Ponieważ komunikacja zachodzi między
 *        procesami przy pomocy nieblokującego pipe'a. Potrzebna jest informacja ile to
 *        jeszcze trzeba pobrać.
 */
struct audio_meta_announcement {
    struct udp_message *message;
    size_t metaint;///< Ile wynosi stała metaint.
    size_t audio_sum;///< Liczba wczytanych bajtów audio od ostatniego segmentu metadanych.
    uint16_t targeted_len;///< Docelowa liczba pobranych bajtów.
    uint16_t fill_ptr;///< Liczba wczytanych bajtów wiadomości.
    bool payload_len_set;///< Czy należy pobrać 2 bajty długości wiadomości.
};

/**
 * Wiadomość IAM.
 */
struct udp_message *gl_iam_message = NULL;

static struct audio_meta_announcement *audio_meta_announcement_new(size_t metaint) {
    struct audio_meta_announcement *announcement = malloc(sizeof(struct audio_meta_announcement));

    if (announcement) {
        bzero(announcement, sizeof(struct audio_meta_announcement));
        announcement->metaint = metaint;

        if ((announcement->message = udp_message_new(PAYLOAD_SIZE + 4)) == NULL) {
            free(announcement);
            announcement = NULL;
        }
    }

    return announcement;
}

static void audio_meta_announcement_delete(struct audio_meta_announcement *announcement) {
    if (announcement) {
        udp_message_delete(announcement->message);
        free(announcement);
    }
}

static void udp_announcement_set_type(struct udp_message *msg, uint16_t htype) {
    uint16_t ntype = htons(htype);
    udp_message_strncmp(msg, 0,(char *) &ntype, 2);
}

static void udp_announcement_set_payload(struct udp_message *msg, uint16_t hlen) {
    uint16_t nlen = htons(hlen);
    udp_message_strncmp(msg, 2, (char *) &nlen, 2);
}

static int set_iam_message(const char *radio_name){
    int result = 0;
    size_t radio_name_length = strlen(radio_name);
    uint16_t buff_size = (radio_name_length < 0xffff-4) ? radio_name_length + 4 : 0xffff;
    struct udp_message *msg = udp_message_new(buff_size);

    if (msg != NULL) {
        udp_announcement_set_type(msg, IAM);
        udp_announcement_set_payload(msg, radio_name_length);
        udp_message_strncmp(msg, 4, radio_name, buff_size - 4);
        gl_iam_message = msg;
    } else {
        result = -1;
    }

    return result;
}

static inline const struct udp_message * get_iam_message(){
    return gl_iam_message;
}

void delete_iam_message() {
    udp_message_delete(gl_iam_message);
}


static int nonblocking_recvfrom(int fd, char *buff, size_t n, struct sockaddr_in *addr, socklen_t *len) {
    struct timeval time = {0,0};
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(fd, &readset);

    if (select(fd + 1, &readset, NULL, NULL, &time) > 0) {
        return recvfrom(fd, buff, n, 0, (struct sockaddr*) addr, len);
    } else {
        return -1;
    }
}

static int receive_annoucement(struct udp_sockets *sockets, struct sockaddr_in *from, socklen_t *len) {
    char buff[5];
    int result = 0;
    int64_t read_result;

    if ((read_result = nonblocking_recvfrom(sockets->unicastfd, buff, 5, from, len)) == 4) {
        uint16_t type = ntohs(*((uint16_t*) buff));
        uint16_t length = ntohs(*(uint16_t*) &buff[2]);

        if (length != 0 || (type != DISCOVER && type != KEEPALIVE)) result = WRONG_ANNOUNCEMENT;
        else result = type;
    } else if (read_result >= 0 ) {
        result = WRONG_ANNOUNCEMENT;
    } else {
        result = read_result;
    }

    return result;
}



static void unicast_handle(int unicastfd, int type, struct sockaddr_in *addr,
                           struct client_set *client_set) {

    if (type == DISCOVER) {
        if (!client_set_contains(client_set, addr, CS_CONNECTED_SET)) {
            const struct udp_message *msg = get_iam_message();
            if (sendto(unicastfd, msg->buff, msg->len, 0, (struct sockaddr*) addr, sizeof(struct sockaddr_in)) < 0) {
                fputs(strerror(errno), stderr);
                exit(1);
            }
            client_set_add_client(client_set, addr);
        } else {
            client_set_refresh_client(client_set, addr, CS_CONNECTED_SET);
        }
    } else if (type == KEEPALIVE && client_set_contains(client_set, addr, CS_CONNECTED_SET)) {
        client_set_refresh_client(client_set, addr, CS_CONNECTED_SET);
    }
}

static void datagram_handle(struct udp_sockets *socket, struct client_set *client_set) {
    uint16_t cnt = 0;
    struct sockaddr_in addr;
    int type;

    do {
        socklen_t len = sizeof(struct sockaddr_in);
        type = receive_annoucement(socket, &addr, &len);
        if (type >= 0) {
            ++cnt;
            unicast_handle(socket->unicastfd, type, &addr, client_set);
        }
    } while (cnt < 3 && type > 0);

}

static int partial_read(int fd, struct audio_meta_announcement *amm) {
    uint16_t start = amm->fill_ptr + 4;
    uint16_t len = amm->targeted_len - amm->fill_ptr;
    int read_result = 0;

    read_result = udp_message_read(amm->message, start, fd, len);
    if (read_result > 0) {
        amm->fill_ptr += read_result;
    } else {
        return READ_ERROR;
    }

    return 0;
}

static int read_targeted_len(int streamfd, struct audio_meta_announcement *amm) {
    if (read(streamfd, &amm->targeted_len, sizeof(uint16_t)) != 2) {
        return READ_ERROR;
    } else {
        amm->payload_len_set = true;
        udp_announcement_set_payload(amm->message, amm->targeted_len);
        return 0;
    }
}

int proxy_handle(int streamfd, int unicastfd, struct client_set *client_set, struct audio_meta_announcement *amm) {
    static int type = AUDIO;

    if (!amm->payload_len_set) {
        udp_announcement_set_type(amm->message, type);
        if (read_targeted_len(streamfd, amm) != 0) return READ_ERROR;
    } else {
        if (amm->fill_ptr < amm->targeted_len) {
            if (partial_read(streamfd, amm) != 0) return -1;
        }
        if (amm->fill_ptr == amm->targeted_len) {
            if (amm->targeted_len > 0) {
                client_set_send_datagram(client_set, unicastfd, amm->message->buff, amm->targeted_len + 4);
                amm->fill_ptr = 0;
                udp_message_clear(amm->message);
            }
            amm->payload_len_set = false;
            if (type == AUDIO && amm->metaint > 0) {
                amm->audio_sum += amm->targeted_len;
                if (amm->audio_sum == amm->metaint) {
                    type = METADATA;
                    amm->audio_sum = 0;
                }
            } else {
                type = AUDIO;
            }
        }
    }

    return 0;
}

int run_proxy_loop(struct udp_sockets *udp_sockets, struct HTTP_response_message *response,
                   int sigfd, int pipefd, int timeout) {
    int ret_code = 0;
    size_t metaint;
    if (HTTP_response_get_icy_metaint(response, &metaint) != 0) metaint = 0;
    const char *radio_name = HTTP_response_get_icy_name(response);

    struct audio_meta_announcement *amm = audio_meta_announcement_new(metaint);
    struct client_set *client_set = client_set_new(timeout);


    if (amm == NULL || client_set == NULL || set_iam_message(radio_name) != 0) {
        goto clean;
    }

    do {
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(sigfd, &readset);
        FD_SET(pipefd, &readset);
        udp_sockets_add_to_fds(udp_sockets, &readset);
        int maxfdp1 = udp_sockets_maxfdp1(udp_sockets, sigfd);
        ret_code = select(maxfdp1, &readset, NULL, NULL, NULL);
        if (ret_code < 0) {
            if (errno == EINTR) continue;
            else break;
        } else if (ret_code > 0) {
            if (FD_ISSET(sigfd, &readset)) {
                break;
            }
            if (FD_ISSET(udp_sockets_get(udp_sockets, T_UNICAST), &readset)) {
                datagram_handle(udp_sockets, client_set);
            }
            if (FD_ISSET(pipefd, &readset)) {
                proxy_handle(pipefd, udp_sockets->unicastfd,
                             client_set, amm);
            }
        }
    } while (true);

clean:
    audio_meta_announcement_delete(amm);
    delete_iam_message();
    client_set_delete(client_set);
    return 0;
}
