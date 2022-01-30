#include <stdatomic.h>
#include <sys/mman.h>

#define STOP_COUNTING 0
#define NEW_INPUT_DEVICE 1

struct asd_event
{
    unsigned char code;
};

typedef struct asd_state
{
    atomic_uint counter;
} asd_state;

void initialize_asd_state(asd_state **state, char *shm_fd)
{
    *state = mmap(NULL, sizeof(asd_state), PROT_READ | PROT_WRITE, MAP_SHARED, atoi(shm_fd), 0);

    if (state == NULL)
    {
        perror("mmap NULL");
    }
}