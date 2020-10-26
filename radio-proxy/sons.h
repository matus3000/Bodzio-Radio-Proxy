#ifndef __SONS__
#define __SONS__
#include <sys/types.h>
struct sons {
    pid_t array[2];
};

void sons_init(struct sons *sons);
pid_t sons_add(struct sons *sons, pid_t son);
pid_t sons_remove(struct sons* sons, pid_t son);
int sons_alive(struct sons *sons);
int sons_kill(struct sons *sons, int signum);
void sons_terminate(struct sons *sons);
int sons_wait(struct sons *sons, int sigchldfd, int timeout);

#endif
