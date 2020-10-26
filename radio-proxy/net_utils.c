#include "net_utils.h"
#include "objective_c/my_string.h"
#include "my_error.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/select.h>
#define MAXLINE 256
#define ASSERT_GE_ZERO(x) if (x < 0) return x;

struct my_stream *my_stream_new(int fd, int signalfd,
                                const struct timeval *_time) {
    if (fd < 0) return NULL;

    struct my_stream *result = malloc(sizeof(struct my_stream));

    if (result) {
        if (_time) {
            if ((result->time = malloc(sizeof(struct timeval))) == NULL) goto error;
            *result->time = *_time;
        } else {
            result->time = NULL;
        }
        result->fd = fd;
        result->signalfd = (signalfd < 0) ? -1 : signalfd;
        result->position = 0;
        result->read_cnt = 0;
        result->eot = false;
        *result->buff = 0;
    }

    return result;

    error:
    free(result);
    return NULL;
}

int my_stream_delete(struct my_stream *my_stream, bool close_fd, bool close_signal) {
    int result = 0;

    if (my_stream) {
        if (close_fd) result |= close(my_stream->fd);
        if (close_signal) result |= close(my_stream->signalfd);
        free(my_stream->time);
        free(my_stream);
    }

    return result;
}

static inline int max(int x, int y) {
    return (x > y) ? x : y;
}
/// Celowo timeval przez wartość. Tak przynajmniej radzą w książce Stevensa.
static int wait_for_read(struct my_stream *my_stream) {
    int result = 0;
    int maxfdp1 = max(my_stream->fd, my_stream->signalfd) + 1;
    struct timeval time;
    struct timeval *timeout = NULL;

    if (my_stream->time) {
        time = *my_stream->time;
        timeout = &time;
    }

    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(my_stream->fd, &readset);
    if (my_stream->signalfd > 0) FD_SET(my_stream->signalfd, &readset);

    result = select(maxfdp1, &readset, NULL, NULL, timeout);

    if (result < 0) {
        result = ERRNO_ERROR;
    } else if (result == 0) {
        result = TIMEOUT;
    } else {
        if (my_stream->signalfd >= 0 && FD_ISSET(my_stream->signalfd, &readset)) {
            result = RECEIVED_SIGNAL;
        } else {
            result = 0;
        }
    }

    return result;
}

static int my_connect(int sockfd, int sigintfd, struct sockaddr *addr, socklen_t len) {
    fd_set readset, writeset;
    int flags = fcntl(sockfd, F_GETFL, 0);
    int n = 0;
    int error = 0;
    ASSERT_GE_ZERO(flags);
    ASSERT_GE_ZERO(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK));

    if ((n = connect(sockfd, (struct sockaddr*)addr, len)) < 0) {
        if (errno != EINPROGRESS) return ERRNO_ERROR;
        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        FD_SET(sockfd, &writeset);
        FD_SET(sockfd, &readset);
        FD_SET(sigintfd, &readset);

        n = select(max(sockfd, sigintfd) + 1, &readset, &writeset, NULL, NULL);
        if (n < 0) return -1;
        if (FD_ISSET(sigintfd, &readset)) {
            return RECEIVED_SIGNAL;
        } else if (FD_ISSET(sockfd, &readset) || FD_ISSET(sockfd, &writeset)) {
            unsigned int size = sizeof(int);
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &size) < 0 ||
                error != 0) return -1;
        } else {
            return -1;
        }
    }

    return 0;
}

static int fill_buffer(struct my_stream *stream) {
    int result = 0;
    if (stream->eot) return 0;
    int16_t read_cnt = stream->read_cnt;

again:
    if ((result = wait_for_read(stream)) != 0) return result;

    if ((read_cnt = read(stream->fd, stream->buff, sizeof(stream->buff))) < 0) {
        if (errno == EINTR)
            goto again;
        else
            return ERRNO_ERROR;
    } else if (read_cnt == 0) {
        stream->eot = true;
        return 0;
    }

    stream->position = 0;
    stream->read_cnt = read_cnt;
    return read_cnt;
}

static int my_read(struct my_stream *stream) {
    int read_cnt = stream->read_cnt;

    if (read_cnt <= 0) {
        read_cnt = fill_buffer(stream);
        if (read_cnt <= 0) return read_cnt;
    }

    --stream->read_cnt;
    return stream->buff[stream->position++];
}


int64_t writen(int fd, int sigfd ,void *buff, size_t n) {
    size_t left;
    size_t written;
    const char *ptr;
    int maxfdp1 = ((fd > sigfd) ? fd : sigfd) + 1;
    fd_set readset;
    fd_set writeset;
    ptr = buff;
    left = n;

    while (left > 0) {
        FD_ZERO(&writeset);
        FD_ZERO(&readset);
        FD_SET(sigfd, &readset);
        FD_SET(fd, &writeset);

        if (select(maxfdp1, &readset, &writeset, NULL, NULL) > 0) {
            if (FD_ISSET(sigfd, &readset)) {
                return n - left;
            } else {
                if ((written = write(fd, ptr, left)) <= 0) {
                    if (errno == EINTR) {
                        written = 0;
                    } else {
                        fputs(strerror(errno), stderr);
                        return ERRNO_ERROR;
                    }
                } else {
                    left -= written;
                    ptr = &ptr[written];
                }
            }
        }
    }

    return n;
}

int read_line(struct my_stream *stream, struct resizable_string *str) {
    int read_result = 0;
    bool CR = false;
    bool LF = false;
    size_t len = base_string_len(str);
    char c;

    while ((!CR || !LF) && (read_result = my_read(stream)) > 0) {
        c = read_result;
        resizable_string_append_c(str, c);

        if (c == '\r') {
            CR = true;
        } else if (c == '\n' && CR == true) {
            LF = true;
        } else {
            CR = false;
            LF = false;
        }

        ++len;
    }

    if (read_result < 0) {
        return read_result;
    } else if (read_result == 0) {
        return READ_LINE_TOO_EARLY;
    } else {
        return 1;
    }
}

int64_t readn(struct my_stream *stream, char *buff, int64_t len) {
    int64_t i = 0;
    int64_t left = len;
    int64_t return_code = 1;

    while (left > 0 && return_code > 0 ) {
        if (stream->read_cnt <= 0) {
            return_code = fill_buffer(stream);
        } else {
            int64_t to_receive = (left < stream->read_cnt) ? left : stream->read_cnt;
            if (buff) {
                memcpy(&buff[i], &stream->buff[stream->position], to_receive);
            }
            left -= to_receive;
            i += to_receive;
            stream->position += to_receive;
            stream->read_cnt -= to_receive;

        }
    }

    if (return_code < 0) {
        return return_code;
    } else {
        return i;
    }
}

/**
 * @brief Funkcja tworzącą aktywny gniazdo TCP, zapożyczona z UNIX
 *        programowanie usług sieciowych.
 */
int tcp_connect(char *host, char *service, int sigfd) {
    int sockfd = -1, n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((n = getaddrinfo(host, service, &hints, &res)) != 0) {
        my_error_set_gai_error(n);
        return GAI_ERROR;
    }

    ressave = res;

    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd >= 0) {
            if (my_connect(sockfd, sigfd, res->ai_addr, res->ai_addrlen) != 0) {
                if (close(sockfd) != 0) exit(1);
                sockfd = TCP_CONNECT_FAIL;
            } else {
                break;
            }
        } else {
            sockfd = ERRNO_ERROR;
        }
    } while ((res = res->ai_next) != NULL);

    freeaddrinfo(ressave);
    return sockfd;
}


size_t find_character(char *w, char c) {
    size_t i = 0;
    while (w[i] != '\0' && w[i] != c) ++i;
    return i;
}

size_t skip_leading_spaces(char *w, size_t beginning) {
    size_t result = beginning;
    while (isspace(w[result])) ++result;
    return result;
}

size_t skip_trailing_spaces(char *w, size_t end) {
    size_t result = end;

    while (isspace(w[result]) && result > 0) --result;

    return result;
}

/**
 * @breif Funkcja pobiera @p len bajtów ze strumienia @p stream i wpisuje
 *        je do deskryptora @p targetfd.
 * @param[in,out] stream
 * @param[out] targetfd
 * @param[in] len
 * @return Liczba przesłanych bajtów. Liczba ujemna jeśli wystąpił błąd.
 */
int64_t readto(struct my_stream *stream, int targetfd, int64_t len) {
    int64_t readed = 0;
    int result = 0;

    do {
        if (stream->read_cnt <= 0) {
            result = fill_buffer(stream);
            if (result < 0) return result;
        }
        int64_t smallread = (len < stream->read_cnt) ? len : stream->read_cnt;
        result = writen(targetfd, stream->signalfd, &stream->buff[stream->position], smallread);
        if (result == smallread) {
                stream->position += result;
                stream->read_cnt -= result;
                len -= smallread;
                readed += result;
        } else if (result > 0) {
            readed += result;
            result = -1;
        } else {
            result = -1;
        }
    } while (len > 0 && result > 0);

    return result > 0 ? readed : -1;
}
