#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <linux/memfd.h>
#include <fcntl.h>

#include "asd_common.h"

#define NEW_PROCESS_ID 0

int main()
{
    char cli_path[PATH_MAX];
    char counter_path[PATH_MAX];

    char *cwd = getcwd(NULL, 0);
    snprintf(counter_path, sizeof(counter_path), "%s/%s", cwd, "counter");
    snprintf(cli_path, sizeof(cli_path), "%s/%s", cwd, "cli");
    free(cwd);

    int pipe_fd[2];
    int *read_fd = &pipe_fd[0];
    int *write_fd = &pipe_fd[1];

    if (pipe(pipe_fd) != 0)
    {
        perror("Failed to open pipe");
        exit(1);
    }

    char readpipe_fd[50];
    char writepipe_fd[50];
    snprintf(readpipe_fd, sizeof(readpipe_fd), "%d", *read_fd);
    snprintf(writepipe_fd, sizeof(writepipe_fd), "%d", *write_fd);

    int fd = memfd_create("", 0);

    char shm_fd[50];

    snprintf(shm_fd, sizeof(shm_fd), "%d", fd);

    if (ftruncate(fd, sizeof(asd_state)) == -1)
    {
        perror("truncate");
    }

    asd_state *state = mmap(NULL, sizeof(asd_state), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    fcntl(*write_fd, F_SETFD, FD_CLOEXEC);
    if (fork() == NEW_PROCESS_ID)
    {
        execlp(counter_path, counter_path, readpipe_fd, shm_fd, NULL);
    }
    fcntl(*write_fd, F_SETFD, fcntl(*write_fd, F_GETFD) & ~FD_CLOEXEC);

    fcntl(*read_fd, F_SETFD, FD_CLOEXEC);
    execlp(cli_path, cli_path, writepipe_fd, shm_fd, NULL);
}
