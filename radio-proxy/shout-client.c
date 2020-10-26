#include "objective_c/my_string.h"
#include "shout_request.h"
#include "shout_response.h"
#include "net_utils.h"
#include "my_error.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#define AUDIO_STATE 0
#define META_STATE 1
#define ERROR_STATE 4
#define END_STATE 5

static bool read_print_continue(int state) {
    return state == AUDIO_STATE || state == META_STATE;
}

static int send_request_message(int sockfd, int sigintfd, struct shout_request_message *rq) {
    int result = 0;
    struct resizable_string *buff = resizable_char_buffer(32);
    if (!buff) return ERRNO_ERROR;

    if ((result = print_shoutcast_request_message(buff, rq)) != 0) {
        goto clean;
    }
    if (writen(sockfd, sigintfd, (void*) base_string_raw(buff), base_string_len(buff)) != base_string_len(buff)) {
        result = UNKNOWN_ERROR;
        goto clean;
    }

clean:
    base_string_delete(buff);
    return result;
}

static int receive_server_response(struct my_stream *stream, struct HTTP_response_message *response) {
    int result = 0;

    result = receive_HTTP_response_message(stream, response);

    return result;
}

int shout_client_server_negotiation(struct my_stream *stream, struct shout_request_message *rq,
                              struct HTTP_response_message *response) {
    int result = 0;
    if ((result = send_request_message(stream->fd, stream->signalfd, rq)) != 0) return -1;
    if ((result = receive_server_response(stream, response)) != 0) return -1;
    return 0;
}

static int get_metadata_size(struct my_stream *stream) {
    uint8_t tmp;
    int result = 0;

    if ((result = readn(stream, (void*)&tmp, sizeof(tmp))) != sizeof(tmp)) {
        if (result >= 0) return UNKNOWN_ERROR;
    }
    else {
        result = tmp * 16;
    }

    return result;
}

static int32_t receive_and_pass_atomic(struct my_stream *stream, int targetfd, bool proxy, uint16_t len) {
    int64_t smallread = len;
    int64_t written = 0;

    if (proxy) {
        uint16_t info = len;
        written = (writen(targetfd, stream->signalfd, &info, sizeof(uint16_t)));
        if (written != sizeof(uint16_t)) return RECEIVED_SIGNAL;
    }

    if (smallread > 0) {
        written = readto(stream, targetfd, smallread);
    } else {
        written = 0;
    }

    return written;
}

static int64_t receive_and_pass_loop(struct my_stream *stream, int targetfd, bool proxy, int64_t len) {
    int64_t to_read = len;
    int64_t readed = 0;
    int64_t smallread = 0;

    while (to_read > 0 && readed == smallread) {
        smallread = (to_read < 256) ? to_read : 256;
        readed = receive_and_pass_atomic(stream, targetfd, proxy, smallread);
        if (readed > 0) to_read -= readed;
    }

    return (readed < 0) ? readed : len - to_read;
}

/**
 * @brief Funkcja odbiera audio ze strumienia @p stream i wypisuje je
 *        na STDIN_FILENO przy pomocy funkcji @ref readto.

 * @return AUDIO_STATE, jeśli metaint wynosi 0 i udało się odczytać i przesłać 256 bajtów dźwięku;
 *         META_STATE, jeśli udało się przesłać metaint bajtów dźwięku i metaint > 0;
 *         END_STATE, jeśli @ref read_to przekazał RECEIVED_SIGNAL;
 *         ERROR_STATE wpp.
 */
static int receive_audio(struct my_stream *stream, size_t metaint, int outfd, bool proxy) {
    int64_t to_read = 0;
    int result = 0;

    if (metaint == 0) {
        to_read = 256;
    } else {
        to_read = metaint;
    }

    if (to_read != receive_and_pass_loop(stream, outfd, proxy, to_read)) {
         result = END_STATE;
    }
    else {
        result = (metaint > 0) ? META_STATE : AUDIO_STATE;
    }

    return result;
}

static int receive_metadata(struct my_stream *stream, int errfd, bool proxy) {
    int64_t return_code;
    uint16_t metadata = get_metadata_size(stream);
    int next_state = AUDIO_STATE;

    return_code = receive_and_pass_atomic(stream, errfd, proxy, metadata);

    if (return_code != metadata) next_state = END_STATE;

    return next_state;
}


int shout_read_write_loop(struct my_stream *stream, size_t metaint, int outfd, int errfd, bool proxy) {
    int state = AUDIO_STATE;

    while (read_print_continue(state)) {
        switch (state) {
        case AUDIO_STATE: state = receive_audio(stream, metaint, outfd, proxy); break;
        case META_STATE: state = receive_metadata(stream, errfd, proxy); break;
        default: state = ERROR_STATE; break;
        }
    }

    return 0;
}
