#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define PIPE_PATH "pipe"
#define PIPE_CHUNK_SIZE (1024 * 4)
#define MSGQ_CHUNK_SIZE (1024 * 8)
#define MSGQ_KEY 80
#define SHM_MSGQ_KEY 91
#define BILLION 1000000000L // 1 billion nanoseconds in a second

static int do_print = 0;

// Function declarations
void shared_memory();
void pipes();
void msg_queue();

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <function_name> <do_print>\n", argv[0]);
        return -1;
    }

    do_print = atoi(argv[2]);

    // Check the string argument and call respective functions
    if (strcmp(argv[1], "shared") == 0)
    {
        shared_memory();
    }
    else if (strcmp(argv[1], "pipes") == 0)
    {
        pipes();
    }
    else if (strcmp(argv[1], "queue") == 0)
    {
        msg_queue();
    }
    else
    {
        printf("Invalid function name: %s. Valid names are 'shared', 'pipes' and 'queue'\n", argv[1]);
        return -1;
    }

    return 0;
}

// Shared memory MSG queue struct
typedef struct SHM_info
{
    long mtype;
    int shm_id;
} SHM_info;

// Experiment for Shared Memory
void shared_memory()
{
    // Access the message queue
    int msqid;
    if ((msqid = msgget(SHM_MSGQ_KEY, IPC_CREAT | 0666)) < 0)
    {
        perror("msgget receiver");
        exit(EXIT_FAILURE);
    }

    // Receive message from the queue
    SHM_info shm_info;
    if (msgrcv(msqid, &shm_info, sizeof(shm_info), 1, 0) < 0)
    {
        perror("msgrcv receiver 1");
        exit(EXIT_FAILURE);
    }

    // Attach to shared memory from id
    void *shmaddr = shmat(shm_info.shm_id, NULL, 0);

    if (shmaddr == (void *)-1)
    {
        perror("Shared memory attach");
        return;
    }

    // printf("Receiver: %02X\n", ((unsigned char *)shmaddr)[777777]);
    
    // End timer after attaching to shared memory
    struct timespec receive_time;
    if (clock_gettime(CLOCK_MONOTONIC, &receive_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    if (do_print) printf("%ld", BILLION * receive_time.tv_sec + receive_time.tv_nsec);

    // Cleanup: Detach from and delete the shared memory segment
    shmdt(shmaddr);
    shmctl(shm_info.shm_id, IPC_RMID, NULL);
}

// Experiment for Pipe communication
void pipes()
{
    int fd;

    // Create or open the FIFO (named pipe) for reading
    if ((fd = open(PIPE_PATH, O_RDONLY)) < 0)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Read message from the FIFO
    int image_size;
    read(fd, &image_size, sizeof(int));
    unsigned char *buffer = malloc(image_size);

    size_t total_read = 0;

    while (total_read < image_size)
    {
        size_t remaining = image_size - total_read;
        size_t read_size = remaining < PIPE_CHUNK_SIZE ? remaining : PIPE_CHUNK_SIZE;

        read(fd, buffer + total_read, read_size);

        total_read += read_size;
    }

    // Get the end time
    struct timespec receive_time;
    if (clock_gettime(CLOCK_MONOTONIC, &receive_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    if (do_print) printf("%ld", BILLION * receive_time.tv_sec + receive_time.tv_nsec);

    free(buffer);

    // Close the FIFO
    close(fd);
}

// Message Queue Struct
typedef struct ImageData
{
    long mtype;
    unsigned char data[MSGQ_CHUNK_SIZE];
} ImageData;

typedef struct DataSize
{
    long mtype;
    int size;
} DataSize;

// Experiment for message queue communication
void msg_queue()
{
    int msqid;
    struct DataSize data_size;
    struct ImageData receive_data;

    struct timespec receive_time;

    // Access the message queue
    if ((msqid = msgget(MSGQ_KEY, IPC_CREAT | 0666)) < 0)
    {
        perror("msgget receiver");
        exit(EXIT_FAILURE);
    }

    // Receive message from the queue
    if (msgrcv(msqid, &data_size, sizeof(data_size.size), 1, 0) < 0)
    {
        perror("msgrcv receiver 1");
        exit(EXIT_FAILURE);
    }

    // Fetch size of image data
    int image_size = data_size.size;
    unsigned char *image_data = (unsigned char *)malloc(image_size);

    size_t total_read = 0;

    // Recieve image data through multiple messages
    while (total_read < image_size)
    {
        size_t remaining = image_size - total_read;
        size_t read_size = remaining < MSGQ_CHUNK_SIZE ? remaining : MSGQ_CHUNK_SIZE;

        // Receive message from the queue
        if (msgrcv(msqid, &receive_data, MSGQ_CHUNK_SIZE, 1, 0) < 0)
        {
            perror("msgrcv receiver 2");
            exit(EXIT_FAILURE);
        }

        // Append the read image chunk onto the buffer which holds all the image data
        memcpy(image_data + total_read, receive_data.data, read_size);

        total_read += read_size;
    }

    // Get the end time
    if (clock_gettime(CLOCK_MONOTONIC, &receive_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    if (do_print) printf("%ld", BILLION * receive_time.tv_sec + receive_time.tv_nsec);

    if (msgctl (msqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(EXIT_FAILURE);
    }
    free(image_data);
}

