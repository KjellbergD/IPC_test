#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define PIPE_PATH "pipe"
#define PIPE_CHUNK_SIZE (1024 * 64)
#define MSGQ_CHUNK_SIZE (1024 * 8)
#define SHM_KEY 200
#define MSGQ_KEY 73
#define BILLION 1000000000L // 1 billion nanoseconds in a second
#define TEST_IMG "img/25KB.png"


// Function declarations
void shared_memory(unsigned char *image_data, int image_size);
void pipes(unsigned char *image_data, int image_size);
void msg_queue(unsigned char *image_data, int image_size);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <function_name>\n", argv[0]);
        return -1;
    }

    // Load test image
    int image_width, image_height, image_channels;
    unsigned char *raw_image_data = stbi_load(TEST_IMG, &image_width, &image_height, &image_channels, STBI_rgb_alpha);

    // Insert image metadata
    int raw_image_size = image_width * image_height * image_channels;
    int image_data_size = raw_image_size + 3 * sizeof(int);
    unsigned char *image_data = (unsigned char *)malloc(image_data_size);
    memcpy(image_data + sizeof(int) * 0, &image_width, sizeof(int));
    memcpy(image_data + sizeof(int) * 1, &image_height, sizeof(int));
    memcpy(image_data + sizeof(int) * 2, &image_channels, sizeof(int));
    memcpy(image_data + sizeof(int) * 3, raw_image_data, raw_image_size);
    stbi_image_free(raw_image_data);

    // Check the string argument and call respective functions
    if (strcmp(argv[1], "shared") == 0)
    {
        shared_memory(image_data, image_data_size);
    }
    else if (strcmp(argv[1], "pipes") == 0)
    {
        pipes(image_data, image_data_size);
    }
    else if (strcmp(argv[1], "queue") == 0)
    {
        msg_queue(image_data, image_data_size);
    }
    else
    {
        printf("Invalid function name: %s. Valid names are 'shared', 'pipes' and 'queue'\n", argv[1]);
        return -1;
    }

    return 0;
}

// Experiment for Shared Memory
void shared_memory(unsigned char *image_data, int image_size)
{
    struct timespec start_time, end_time;
    
    // Get the start time
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    
    int shmid = shmget(SHM_KEY, image_size, IPC_CREAT | 0666);
    char *shmaddr = shmat(shmid, NULL, 0);

    // Copy image data to shared memory segment
    memcpy(shmaddr, image_data, image_size);

    // Get the end time
    if (clock_gettime(CLOCK_MONOTONIC, &end_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    // Calculate time it took to create, attach and copy to shared memory
    long elapsed_time_ns = (end_time.tv_sec - start_time.tv_sec) \
            * BILLION + (end_time.tv_nsec - start_time.tv_nsec);

    printf("%ld", elapsed_time_ns);

    // Detach from shared memory
    shmdt(shmaddr);
}

// Experiment for Pipe communication
void pipes(unsigned char *image_data, int image_size)
{
    int fd;
    struct timespec send_time;

    // Create or open the FIFO (named pipe) for writing
    if ((fd = open(PIPE_PATH, O_WRONLY)) < 0)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Get the start time
    if (clock_gettime(CLOCK_MONOTONIC, &send_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    printf("%ld", BILLION * send_time.tv_sec + send_time.tv_nsec);
    // printf("Send time was: %llu \n", send_time.tv_nsec);

    // Write message to the FIFO
    write(fd, &image_size, sizeof(int));

    size_t total_sent = 0;

    while (total_sent < image_size)
    {
        size_t remaining = image_size - total_sent;
        size_t send_size = remaining < PIPE_CHUNK_SIZE ? remaining : PIPE_CHUNK_SIZE;

        write(fd, image_data + total_sent, send_size);

        total_sent += send_size;
    }

    // printf("Sender sent message: %s\n", message);

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
void msg_queue(unsigned char *image_data, int image_size)
{
    struct timespec send_time;

    // Get the start time
    if (clock_gettime(CLOCK_MONOTONIC, &send_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    printf("%ld", BILLION * send_time.tv_sec + send_time.tv_nsec);
    
    // Create or access the message queue
    int msqid;
    if ((msqid = msgget(MSGQ_KEY, IPC_CREAT | 0666)) < 0) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    
    DataSize data_size;
    data_size.mtype = 1;
    data_size.size = image_size;

    // Send message to the queue
    if (msgsnd(msqid, &data_size, sizeof(int), 0) < 0) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }

    size_t total_sent = 0;

    // Send image data in multple messages
    while (total_sent < image_size)
    {
        size_t remaining = image_size - total_sent;
        size_t send_size = remaining < MSGQ_CHUNK_SIZE ? remaining : MSGQ_CHUNK_SIZE;

        ImageData send_data;
        send_data.mtype = 1;
        memcpy(send_data.data, image_data + total_sent, send_size);

        // Send message to the queue
        if (msgsnd(msqid, &send_data, MSGQ_CHUNK_SIZE, 0) < 0) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }

        total_sent += send_size;
    }
}