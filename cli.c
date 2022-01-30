#include <stdio.h>
#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <time.h>

#include <sys/mman.h>
#include <wait.h>

#include "asd_common.h"
#include "emoji_data_structure.h"

#define DEVICE_BY_PATH_FILES_PATH "/dev/input/by-path/"
#define DEVICE_BY_ID_FILES_PATH "/dev/input/by-id/"
#define DEVICE_FILES_PATH "/dev/input/"
#define SYMBOLIC_LINK DT_LNK
#define MAX_FILE_NAME_STRING_LENGTH (NAME_MAX + 1)
#define JUMP_TO_END_OF_PREVIOUS_LINE "\33[1F"
#define CLEAR_CURRENT_LINE "\33[2K\r"
#define UPDATES_PER_SECOND 60
#define MEASURE_INTERVAL_SECONDS 2
#define SECOND_NS 1000000000

void initialize_counter_pipe(char *write_pipe_fd, int *counter_pipe_write);

typedef struct device
{
    char absolute_path[PATH_MAX];
    char name[MAX_FILE_NAME_STRING_LENGTH];
} device;

typedef struct
{
    uint64_t n;
    device *devices;
} input_devices;

uint64_t
devices_count(DIR *directory)
{
    uint64_t counter = 0;

    struct dirent *file;

    while ((file = readdir(directory)) != NULL)
    {
        if (file->d_type == SYMBOLIC_LINK)
        {
            counter++;
        }
    }

    rewinddir(directory);

    return counter;
}

void store_device_info(struct device *device, struct dirent *file, const char *folder)
{
    char selected_device_path[PATH_MAX - strlen(folder)];
    struct stat sb = {0};
    char linkname[PATH_MAX] = {0};

    strcpy(device->name, file->d_name);

    sprintf(selected_device_path, "%s%s", folder, device->name);
    lstat(selected_device_path, &sb);

    if (readlink(selected_device_path, linkname, sb.st_size + 1) < 0)
    {
        perror("Failed to readlink");
        exit(1);
    }
    linkname[sb.st_size] = '\0';

    sprintf(selected_device_path, "%s%s", folder, linkname);
    if (realpath(selected_device_path, device->absolute_path) == NULL)
    {
        perror("Failed to realpath");
        exit(1);
    }
}

input_devices *list_devices()
{
    DIR *by_path_directory;
    DIR *by_id_directory;

    by_id_directory = opendir(DEVICE_BY_ID_FILES_PATH);
    by_path_directory = opendir(DEVICE_BY_PATH_FILES_PATH);
    uint64_t by_id_count = devices_count(by_id_directory);
    uint64_t by_path_count = devices_count(by_path_directory);

    input_devices *input_devices = malloc(sizeof(*input_devices));
    input_devices->n = by_id_count + by_path_count;

    input_devices->devices = malloc(input_devices->n * sizeof(device));
    memset(input_devices->devices, 0, sizeof(input_devices->n * sizeof(device)));

    struct dirent *file;

    for (uint64_t i = 0; ((file = readdir(by_path_directory)) != NULL) && i < by_path_count;)
    {
        if (file->d_type == SYMBOLIC_LINK)
        {
            device *device = &input_devices->devices[i];

            store_device_info(device, file, DEVICE_BY_PATH_FILES_PATH);

            i++;
        }
    }

    for (uint64_t i = by_path_count; ((file = readdir(by_id_directory)) != NULL) && i < input_devices->n;)
    {
        if (file->d_type == SYMBOLIC_LINK)
        {
            device *device = &input_devices->devices[i];

            store_device_info(device, file, DEVICE_BY_ID_FILES_PATH);

            i++;
        }
    }

    closedir(by_path_directory);
    closedir(by_id_directory);

    return input_devices;
}

device *device_selector(input_devices *devices)
{

    printf("Select one of the input devices:\n");

    for (size_t i = 0; i < devices->n; i++)
    {
        printf("%ld %s\n", i + 1, devices->devices[i].name);
    }

    uint64_t device_number = -1;
    printf("\n");
    printf(JUMP_TO_END_OF_PREVIOUS_LINE);

    while (0 == device_number || devices->n < device_number)
    {
        if (scanf("%ld", &device_number) != 1)
        {
            perror("Failed to scanf");
            exit(1);
        }

        printf(JUMP_TO_END_OF_PREVIOUS_LINE);
        printf(CLEAR_CURRENT_LINE);
    }

    for (size_t i = 0; i < devices->n; i++)
    {
        printf(CLEAR_CURRENT_LINE);
        printf(JUMP_TO_END_OF_PREVIOUS_LINE);
    }

    printf(CLEAR_CURRENT_LINE);
    printf(JUMP_TO_END_OF_PREVIOUS_LINE);
    printf(CLEAR_CURRENT_LINE);

    return &devices->devices[device_number - 1];
}

void time_difference(const struct timespec *start, const struct timespec *end, struct timespec *result)
{
    if (start->tv_nsec > end->tv_nsec)
    {
        result->tv_sec = end->tv_sec - start->tv_sec - 1;
        result->tv_nsec = end->tv_nsec - start->tv_nsec + SECOND_NS;
    }
    else
    {
        result->tv_sec = end->tv_sec - start->tv_sec;
        result->tv_nsec = end->tv_nsec - start->tv_nsec;
    }
}

void time_to_sleep(const struct timespec *update_period, struct timespec *elapsed_time)
{
    if (elapsed_time->tv_sec > update_period->tv_sec)
    {
        *elapsed_time = (struct timespec){0, 0};
        printf("aaaaa\n");
    }
    else if (elapsed_time->tv_sec == update_period->tv_sec && elapsed_time->tv_nsec > update_period->tv_nsec)
    {
        *elapsed_time = (struct timespec){0, 0};
        printf("aaaaa\n");
    }
    else
    {
        if (elapsed_time->tv_nsec > update_period->tv_nsec)
        {

            elapsed_time->tv_sec = update_period->tv_sec - elapsed_time->tv_sec - 1;
            elapsed_time->tv_nsec = update_period->tv_nsec - elapsed_time->tv_nsec + SECOND_NS;
        }
        else
        {
            elapsed_time->tv_sec = update_period->tv_sec - elapsed_time->tv_sec;
            elapsed_time->tv_nsec = update_period->tv_nsec - elapsed_time->tv_nsec;
        }
    }
}

int main(int argc, char *argv[])
{
    int counter_pipe_write = atoi(argv[1]);
    asd_state *state = NULL;
    initialize_asd_state(&state, argv[2]);

    input_devices *input_devices = list_devices();
    device *selected_device = device_selector(input_devices);

    int nbytes;
    nbytes = write(counter_pipe_write, &(struct asd_event){.code = NEW_INPUT_DEVICE}, sizeof(struct asd_event));
    nbytes = write(counter_pipe_write, &selected_device->absolute_path, strlen(selected_device->absolute_path) + 1);

    struct timespec start = {0, 0}, end = {0, 0}, result = {0, 0};
    const struct timespec update_period = {(1 / UPDATES_PER_SECOND) / 1, (long long int)(SECOND_NS / UPDATES_PER_SECOND) % SECOND_NS};

    double old_average = 0;

    printf("%f presses/s %s\n", 0.0, get_emoji(0));
    for (clock_gettime(CLOCK_MONOTONIC, &start);; clock_gettime(CLOCK_MONOTONIC, &start))
    {
        unsigned int presses = atomic_exchange(&state->counter, 0);
        double average = ((double)presses / ((double)1 / UPDATES_PER_SECOND)) / (MEASURE_INTERVAL_SECONDS * UPDATES_PER_SECOND) + old_average / ((MEASURE_INTERVAL_SECONDS * UPDATES_PER_SECOND) / (double)(MEASURE_INTERVAL_SECONDS * UPDATES_PER_SECOND - 1));
        old_average = average;

        printf(JUMP_TO_END_OF_PREVIOUS_LINE);
        printf(CLEAR_CURRENT_LINE);
        printf(JUMP_TO_END_OF_PREVIOUS_LINE);
        printf(CLEAR_CURRENT_LINE);

        if (printf("% 5.01f presses/s %s\n", average, get_emoji(average)) < 0)
        {
            perror("printf");
        }
        printf("% 3ld %09ld\n", result.tv_sec, result.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &end);
        time_difference(&start, &end, &result);
        time_to_sleep(&update_period, &result);
        nanosleep(&result, NULL);
    }

    close(counter_pipe_write);
    free(input_devices->devices);
    free(input_devices);

    wait(NULL);
}
