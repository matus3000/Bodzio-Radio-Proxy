#include "sons.h"
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#define ERROR 1
#define TIME_OUT 2


void sons_init(struct sons *sons) {
    sons->array[0] = -1;
    sons->array[1] = -1;
}

pid_t sons_add(struct sons *sons, pid_t son) {
    int result = son;

    if (result <= 0) return result;

    if (sons->array[0] == -1) {
        sons->array[0] = son;
    } else if (sons->array[1] == -1) {
        sons->array[1] = son;
    } else {
        result = ERROR;
    }

    return result;
}

pid_t sons_remove(struct sons* sons, pid_t son) {
    if (son <= 0) return ERROR;
    pid_t result = son;

    if (sons->array[0] == son) {
        sons->array[0] = -1;
    } else if (sons->array[1] == son) {
        sons->array[1] = -1;
    } else {
        result = ERROR;
    }

    return result;
}

int sons_kill(struct sons *sons, int signum) {
    int return_code = 0;

    for (int i = 0; i < 2; ++i) {
        if (sons->array[i] > 0) return_code |= kill(sons->array[i], signum);
    }

    return return_code;
}

int sons_alive(struct sons *sons) {
    int result = 0;
    if (sons->array[0] > 0) ++result;
    if (sons->array[1] > 0) ++result;

    return result;
}

void sons_terminate(struct sons *sons) {
    sons_kill(sons, SIGKILL);

    while (wait(NULL) > 0) ;

    sons_init(sons);
}

static int sigchldfd_read_and_wait(int sigchldfd, struct sons *sons) {
    int result = 0;
    size_t read_bytes;
    pid_t pid;
    int stat;
    struct signalfd_siginfo sig;

    if ((read_bytes = read(sigchldfd, &sig, sizeof(struct signalfd_siginfo)))
        == sizeof(struct signalfd_siginfo)) {
        while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) sons_remove(sons, pid);
    } else {
        result = ERROR;
    }

    return result;
}

int sons_wait(struct sons *sons, int sigchldfd, int timeout) {
    int result = 0;
    int alive = sons_alive(sons);
    time_t start = time(NULL);
    time_t current_time;
    int left = timeout;

    while (result == 0 && alive > 0) {
        fd_set readset;
        struct timeval select_timeout;

        select_timeout.tv_sec = left;
        select_timeout.tv_usec = 0;

        FD_ZERO(&readset);
        FD_SET(sigchldfd, &readset);
        if ((result = select(sigchldfd + 1, &readset, NULL, NULL, &select_timeout)) == 1) {
            if (FD_ISSET(sigchldfd, &readset)) {
                result = sigchldfd_read_and_wait(sigchldfd, sons);
                alive = sons_alive(sons);
            } else {
                result = ERROR;
            }
        } else if (result == 0) {
            result = TIME_OUT;
        } else {
            result = ERROR;
        }

        left = (start + timeout + 1 > (current_time = time(NULL)))
            ? start + timeout - current_time
            : 0;
    }


    return (result == TIME_OUT) ? 0 : result;
}
