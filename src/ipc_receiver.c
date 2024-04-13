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
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define PIPE_PATH "pipe"
#define PIPE_CHUNK_SIZE (1024 * 4)
#define MSGQ_CHUNK_SIZE (1024 * 8)
#define SHM_KEY 200
#define MSGQ_KEY 80
#define BILLION 1000000000L // 1 billion nanoseconds in a second

static int do_print = 0;

// Function declarations
void shared_memory();
void pipes();
void msg_queue();
void save_image(unsigned char *image_data);

int main(int argc, char *argv[])
{
    sleep(1);
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

// Experiment for Shared Memory
void shared_memory()
{
    struct timespec before_attach_time, after_attach_time;

    // Recieve shared memory id from recieved data
    int shmid;

    // Start timer before attaching to shared memory segment
    if (clock_gettime(CLOCK_MONOTONIC, &before_attach_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    if ((shmid = shmget(SHM_KEY, 0, 0)) == -1)
    {
        perror("Shared memory get");
        return;
    }

    // Attach to shared memory from id
    void *shmaddr = shmat(shmid, NULL, 0);

    if (shmaddr == (void *)-1)
    {
        perror("Shared memory attach");
        return;
    }

    // End timer after attaching to shared memory
    if (clock_gettime(CLOCK_MONOTONIC, &after_attach_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    // Calculate time it took to attach to shared memory segment
    long elapsed_time_ns = ((after_attach_time.tv_sec - before_attach_time.tv_sec) \
            * BILLION) + (after_attach_time.tv_nsec - before_attach_time.tv_nsec);

    if (do_print) printf("%ld", elapsed_time_ns);

    save_image(shmaddr);

    // Cleanup: Detach from and delete the shared memory segment
    shmdt(shmaddr);
    shmctl(shmid, IPC_RMID, NULL);
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

    save_image(buffer);
    free(buffer);
    // printf("Recieve time was: %llu \n", receive_time.tv_nsec);

    // printf("Receiver received message: %s\n", buffer);

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
    save_image(image_data);
    free(image_data);
}

void save_image(unsigned char *image_data)
{
    return;
    int image_width = *(int *)image_data;
    int image_height = *((int *)(image_data + sizeof(int)));
    int image_channels = *((int *)(image_data + sizeof(int) * 2));

    int stride = image_width * image_channels * sizeof(uint8_t);
    int success = stbi_write_png("output.png", image_width, image_height, image_channels, image_data + sizeof(int) * 3, stride);
    if (!success)
    {
        fprintf(stderr, "Error writing image\n");
    }
}
