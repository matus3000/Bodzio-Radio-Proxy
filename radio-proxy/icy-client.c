#include "objective_c/my_string.h"
#include "shout_request.h"
#include "shout_response.h"
#include "net_utils.h"
#include "my_error.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define AUDIO_STATE 0
#define META_STATE 1
#define ERROR_STATE 4
#define END_STATE 5

struct client_args {
    bool proxy;
    bool metadata;
    int fd_out;
    int fd_err;
    size_t metaint;
};

bool read_print_continue(int state) {
    return state != ERROR_STATE && state != END_STATE;
}

int send_request_message(int sockfd, struct shout_request_message *rq) {
    int result = 0;
    struct resizable_string *buff = resizable_char_buffer(32);
    if (!buff) return 1;

    if (print_shoucast_request_message(buff, rq) != 0) {
        result = 1;
        goto clean;
    }
    if (writen(sockfd, (void*) base_string_raw(buff), base_string_len(buff)) != base_string_len(buff)) {
        result = 1;
        goto clean;
    }

clean:
    delete(buff);
    return result;
}

int receive_server_response(struct my_stream *stream, struct HTTP_response_message **response) {
    *response = malloc(sizeof(struct HTTP_response_message));

    if (!response) return ERRNO_ERROR;

    HTTP_response_init(*response);
    if (receive_HTTP_response_message(stream, *response) == 0) {
        return 0;
    } else {
        HTTP_response_destroy(*response);
        *response = NULL;
        return -1;
    }
}

int client_server_negotiation(struct my_stream *stream, struct shout_request_message *rq,
                              struct HTTP_response_message **response) {
    int result = 0;

    if ((result = send_request_message(stream->fd, rq)) != 0) return -1;
    if ((result = receive_server_response(stream, response)) != 0) return -1;
    return 0;
}

int get_metadata_size(struct my_stream *stream) {
    uint8_t tmp;
    int result = 0;

    if (readn(stream, (void*)&tmp, 8) != 8) return -1;
    result = tmp * 16;

    return result;
}

int receive_audio(struct my_stream *stream, struct client_args *s) {
    int64_t return_code = 0;

    if (!s->metadata){
        return_code = readto(stream, s->fd_out, 256);
        return (return_code != 256) ? ERROR_STATE : AUDIO_STATE;
    } else {
        return_code = readto(stream, s->fd_out, s->metaint);
        return (return_code == (int64_t)s->metaint) ?
            META_STATE :
            ERROR_STATE;
    }
}

int receive_metatdata(struct my_stream *stream, struct client_args *args) {
    int64_t return_code;
    int metadata = get_metadata_size(stream);

    if (metadata > 0) {
        return_code = readto(stream, args->fd_err, metadata);
        return (return_code == metadata) ? AUDIO_STATE : ERROR_STATE;
    } else {
        return AUDIO_STATE;
    }
}


int run_client(struct my_stream *stream, struct client_args *args) {
    int state = AUDIO_STATE;

    while (read_print_continue(state)) {
        switch (state) {
        case AUDIO_STATE: state = receive_audio(stream, args);
        case META_STATE: state = receive_metatdata(stream, args);
        default: state = ERROR_STATE;
        }
    }
    return 0;
}
