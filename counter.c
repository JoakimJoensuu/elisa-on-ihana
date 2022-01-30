#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <linux/input.h>

#include "asd_common.h"

#define KEY_PRESS 1

#define CLI_INPUT 0
#define INPUT_DEVICE 1

void handle_new_input_device(int fd, struct pollfd pollfd_list[], int *initialized_fds)
{
    char new_input_device[PATH_MAX];

    int nbytes = read(fd, new_input_device, sizeof(new_input_device));

    if (nbytes <= 0)
    {
        perror("Error reading new_input_device.\n");
    }

    (&pollfd_list[INPUT_DEVICE])->fd = open(new_input_device, O_RDONLY | O_NONBLOCK);

    if (pollfd_list[INPUT_DEVICE].fd == -1)
    {
        perror("Failed to open input_device. Maybe you should use sudo?");
        exit(1);
    }

    (&pollfd_list[INPUT_DEVICE])->events = POLLIN;

    (*initialized_fds)++;
}

void handle_asd_event(struct asd_event *event, int fd, struct pollfd pollfd_list[], int *initialized_fds)
{
    switch (event->code)
    {
    case NEW_INPUT_DEVICE:
        handle_new_input_device(fd, pollfd_list, initialized_fds);
        break;
    default:
        printf("UNIMPLEMENTED ASD_EVENT HANDLER!\n");
        break;
    }
}

void check_cli_fd(struct pollfd *pollfd, struct pollfd pollfd_list[], int *initialized_fds)
{
    switch (pollfd->revents)
    {
    case 0:
        break;
    case POLLIN:
        struct asd_event asd_event;
        int nbytes = read(pollfd->fd, &asd_event, sizeof(asd_event));

        if (nbytes != sizeof(asd_event))
        {
            printf("CLI sent something to counter but message size was wrong (%d, should be %ld)\n", nbytes, sizeof(asd_event));
            exit(1);
        }

        handle_asd_event(&asd_event, pollfd->fd, pollfd_list, initialized_fds);

        break;
    case POLLHUP:
        printf("CLI closed it's end of the pipe. Maybe we should just give up and exit.\n");
        exit(1);
    default:
        printf("UNIMPLEMENTED FOR REVENT Ox%4x\n", pollfd->revents);
        break;
    }
}

void handle_input_event(struct input_event *input_data, asd_state *state)
{
    if (input_data->type & EV_KEY & input_data->value) //(input_data->type == EV_KEY && input_data->value == KEY_PRESS)
    {
        atomic_fetch_add(&state->counter, 1);
    }
}

void check_input_device(struct pollfd *pollfd, asd_state *state)
{
    switch (pollfd->revents)
    {
    case 0:
        break;
    case POLLIN:
        struct input_event input_data;
        int nbytes = read(pollfd->fd, &input_data, sizeof(input_data));

        if (nbytes != sizeof(input_data))
        {
            printf("READ INVALID NUMBER OF BYTES FROM INPUT DEVICE (%d)", nbytes);
        }

        handle_input_event(&input_data, state);

        break;
    default:
        printf("UNIMPLEMENTED FOR REVENT Ox%4x\n", pollfd->revents);
        break;
    }
}

void initialize_cli_pipe(char *read_pipe_fd, struct pollfd pollfd_list[], int *initialized_fds)
{
    struct pollfd *pollfd = &pollfd_list[CLI_INPUT];

    pollfd->fd = atoi(read_pipe_fd);
    pollfd->events = POLLIN;
    pollfd->revents = 0;

    (*initialized_fds)++;
}

int main(int argc, char *argv[])
{
    struct pollfd pollfd_list[2] = {0};
    int initialized_fds = 0;
    asd_state *state = NULL;

    initialize_cli_pipe(argv[1], pollfd_list, &initialized_fds);
    initialize_asd_state(&state, argv[2]);
    nfds_t asd;
    for (;;)
    {
        int n = poll(pollfd_list, sizeof(pollfd_list) / sizeof(struct pollfd), -1);

        switch (n)
        {
        case -1:
            perror("Polling error\n");
            exit(1);
            break;
        case 0:
            printf("Zero file descriptors ready, this should not happen.\n");
            break;
        default:
            check_cli_fd(&pollfd_list[CLI_INPUT], pollfd_list, &initialized_fds);
            check_input_device(&pollfd_list[INPUT_DEVICE], state);
            break;
        }
    }
}
