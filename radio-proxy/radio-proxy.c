#include "net_utils.h"
#include "rp_command_line.h"
#include "shout-client.h"
#include "proxy-client.h"
#include "my_error.h"
#include "sons.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <fcntl.h>
#define CMD_ARGS_NUM 5
#define ERROR_CODE 1

#define FCNTL_SET_FLAG(fd, flagvalue, goto_label)                       \
    {                                                                   \
        int flags = fcntl(fd, F_GETFL, 0);                              \
        if (flags < 0 || fcntl(fd, F_SETFL, flags | flagvalue) != 0) goto goto_label; \
    }


static int set_signalfd(int fd, int sig) {
    int result = 0;
    sigset_t sigset;
    if (sigemptyset(&sigset) != 0) return -1;
    if (sigaddset(&sigset, sig) != 0) return -1;

    result = signalfd(fd, &sigset, 0);

    return result;
}

static int block_signal(int sig) {
    sigset_t sigset;
    if (sigemptyset(&sigset) != 0) return -1;
    if (sigaddset(&sigset, sig) != 0) return -1;
    return sigprocmask(SIG_BLOCK, &sigset, NULL);
}


static int connect_to_radio(char *host, char *port, int sigintfd) {
    int sockfd = -1;
    sockfd = tcp_connect(host, port, sigintfd);
    return sockfd;
}

static int negotiate_with_radio(struct my_stream *stream, const struct command_line_args *cmd,
                         struct HTTP_response_message *server_response) {
    int res = 0;
    struct shout_request_message rq;

    rq.metadata = cmd->metadata;
    rq.request_target = cmd->resource;
    rq.uri_host = NULL;

    res = shout_client_server_negotiation(stream, &rq, server_response);
    if (res == 0 && !cmd->metadata) {
        size_t metaint;
        if (HTTP_response_get_icy_metaint(server_response, &metaint) == 0 && metaint != 0) {
            res = -1;
        }
    }

    return res;
}


static pid_t run_proxy_server_process(const struct command_line_args *cmd, int sigfd, int writefd,
                                struct HTTP_response_message *response) {
    int ret_code;
    pid_t result = -1;
    int sockfd;
    size_t metaint = 0;
    struct my_stream *stream = NULL;

    if ((sockfd = connect_to_radio(cmd->host, cmd->port, sigfd)) < 0) return sockfd;
    if ((stream = my_stream_new(sockfd, sigfd, &cmd->timeout)) == NULL) {
        result = -1;
        goto clean;
    }
    if ((ret_code = negotiate_with_radio(stream, cmd, response)) != 0) {
        result = ret_code;
        goto clean;
    }
    HTTP_response_get_icy_metaint(response, &metaint);

    if ((result = fork()) == 0) {
        int outfd, errfd;
        bool proxy;
        int my_sigint = set_signalfd(-1, SIGINT);
        dup2(sigfd, my_sigint);

        if (writefd > 0) {
            outfd = writefd;
            errfd = writefd;
            proxy = true;
        } else {
            outfd = STDOUT_FILENO;
            errfd = STDERR_FILENO;
            proxy = false;
        }

        ret_code = shout_read_write_loop(stream, metaint, outfd, errfd, proxy);
    }

clean:
    close(sockfd);
    if (stream) my_stream_delete(stream, false, false);
    return result;
}

static pid_t run_proxy_client_process(const struct command_line_args *cmd, int sigfd, int readfd,
                               struct HTTP_response_message *response) {
    pid_t result;

    if ((result = fork()) == 0) {
        struct udp_sockets *udp_sockets = udp_sockets_new(cmd->proxy_port, cmd->proxy_group);
        if (udp_sockets == NULL) exit(1);
        run_proxy_loop(udp_sockets, response, sigfd, readfd, cmd->proxy_timeout);
        udp_sockets_delete(udp_sockets);
        exit(0);
    }

    return result;
}

static int create_pipe(int pipefd[]) {
    int result = 0;
    if ((result = pipe(pipefd)) == 0) {
        FCNTL_SET_FLAG(pipefd[0], O_NONBLOCK, clean);
        FCNTL_SET_FLAG(pipefd[1], O_NONBLOCK, clean);
    }

    return result;
clean:
    close(pipefd[0]);
    close(pipefd[1]);
    pipefd[0] = -1;
    pipefd[1] = -1;
    return -1;
}

int run_proxy_service(struct command_line_args *cmd, int sigfd, struct sons *sons) {
    int result = 0;
    int pipefd[2] = {-1, -1};
    struct HTTP_response_message response;
    pid_t fork_result;

    if (cmd->proxy_port > 0 && create_pipe(pipefd) != 0) return 1;
    if (HTTP_response_init(&response) != 0) return 1;

    if ((fork_result = run_proxy_server_process(cmd, sigfd, pipefd[1], &response)) <= 0) {
        result = fork_result;
        goto clean;
    } else {
        sons_add(sons, fork_result);
    }

    if (pipefd[0] > 0) {
        if ((fork_result = run_proxy_client_process(cmd, sigfd, pipefd[0] ,&response)) < 0) {
            result = fork_result;
            sons_terminate(sons);
        } else if (fork_result == 0) {
            sons_init(sons);
        } else {
            sons_add(sons, fork_result);
        }
    }


clean:
    // Celowo zamykamy tak późno pipe'y, aby uniknąć SIGPIPE'a.
    HTTP_response_destroy(&response);
    if (pipefd[0] != -1) close(pipefd[0]);
    if (pipefd[1] != -1) close(pipefd[1]);
    return result;
}

/**
 * @brief Funkcja oczekuje na sygnał SIGINT lub SIGCHLD.
 *        Następnie wysyła do @p sons sygnał SIGINT i oczekuje na
 *        na ich zakończenie. Jeśli są uparci wysyłya SIGKILL.
 *
 * @return 0 jeśli otrzymano sygnał SIGINT i nie trzeba było korzystać z SIGKILL,
 *         aby zabić synów. Liczba różna od 0, jeśli otrzymano sygnał SIGCHLD przed
 *         sygnałem SIGINT.
 */
static int main_wait(int sigintfd, struct sons* sons) {
    if (sons_alive(sons) <= 0) return 0;
    int return_code = 0;
    int sigchldfd;
    int maxfdp1;
    fd_set readset;

    if ((sigchldfd = set_signalfd(-1, SIGCHLD)) < 0) {
        sons_terminate(sons);
        return UNKNOWN_ERROR;
    }

    maxfdp1 = ((sigintfd > sigchldfd) ? sigintfd : sigchldfd) + 1;
    FD_ZERO(&readset);
    FD_SET(sigintfd, &readset);
    FD_SET(sigchldfd, &readset);

    if (select(maxfdp1, &readset, NULL, NULL, NULL) <= 0) {
        return_code = -1;
        sons_terminate(sons);
    } else {
        if (FD_ISSET(sigchldfd, &readset)) {
            sons_wait(sons, sigchldfd, 0);
            return_code = -1;
        }
        sons_kill(sons, SIGINT);
        sons_wait(sons, sigchldfd, 60);
        if (sons_alive(sons) > 0) {
            sons_terminate(sons);
            return_code = -1;
        }
    }

    close(sigchldfd);
    return return_code;
}

int main(int argc, char **argv) {
    int result = 0;
    int sigintfd = -1;
    struct command_line_args cmd;
    struct sons sons;

    sons_init(&sons);
    if (parse_command_line_args(argc, argv, &cmd) != 0) return ERROR_CODE;
    if (block_signal(SIGINT) || block_signal(SIGCHLD) != 0) return ERROR_CODE;
    if ((sigintfd = set_signalfd(-1, SIGINT)) < 0) return ERROR_CODE;

    result = run_proxy_service(&cmd, sigintfd, &sons) |
        main_wait(sigintfd, &sons);

    if (sigintfd > 0) close(sigintfd);
    return result == 0 ? 0 : ERROR_CODE;
}
