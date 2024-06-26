#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define PIPE_PATH "pipe"
#define PIPE_CHUNK_SIZE (1024 * 4)
#define MSGQ_CHUNK_SIZE (1024 * 8)
#define MSGQ_KEY 80
#define SHM_MSGQ_KEY 91
#define BILLION 1000000000L // 1 billion nanoseconds in a second
#define TEST_IMG "img/JEPPE.PNG"
#define ONE_MB (1024 * 1024)

static int do_print = 0;

// Function declarations
void shared_memory(unsigned char *image_data, int image_size);
void pipes(unsigned char *image_data, int image_size);
void msg_queue(unsigned char *image_data, int image_size);


void fill_random_data(unsigned char* buffer, size_t size) {
    srand(time(NULL)); // Seed the random number generator
    for (size_t i = 0; i < size; i++) {
        buffer[i] = rand() % 256;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <function_name> <do_print> <num_images>\n", argv[0]);
        return -1;
    }

    do_print = atoi(argv[2]);
    int num_images = atoi(argv[3]);

    // unsigned char one_mb_image_data[ONE_MB];


    // unsigned char data_pattern = 0xAB;
    // for (int i = 0; i < ONE_MB; i++) {
    //     one_mb_image_data[i] = data_pattern;
    // }

    // int fake_width = 1024;
    // int fake_height = 1024;
    // int fake_channels = 1;
    int image_data_size = ONE_MB * num_images;
    // int offset = 0;
    unsigned char *image_data = (unsigned char *)malloc(image_data_size);
    fill_random_data(image_data, image_data_size);
    // for (size_t i = 0; i < num_images; i++)
    // {
    //     memcpy(image_data + offset, &fake_width, sizeof(int)); offset += sizeof(int);
    //     memcpy(image_data + offset, &fake_height, sizeof(int)); offset += sizeof(int);
    //     memcpy(image_data + offset, &fake_channels, sizeof(int)); offset += sizeof(int);
    //     memcpy(image_data + offset, one_mb_image_data, ONE_MB); offset += ONE_MB;
    // }
    
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

    free(image_data);

    return 0;
}

// Experiment for Shared Memory

// Shared memory MSG queue struct
typedef struct SHM_info
{
    long mtype;
    int shm_id;
} SHM_info;

void shared_memory(unsigned char *image_data, int image_size)
{
    
    // Get the start time
    struct timespec send_time;
    if (clock_gettime(CLOCK_MONOTONIC, &send_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    
    if (do_print) printf("%ld", BILLION * send_time.tv_sec + send_time.tv_nsec);
    
    int shmid = shmget(send_time.tv_nsec, image_size, IPC_CREAT | 0666);
    unsigned char *shmaddr = shmat(shmid, NULL, 0);

    // printf("Sender before memcpy: %02X\n", shmaddr[777777]);
    
    // Copy image data to shared memory segment
    memcpy(shmaddr, image_data, image_size);    

    // printf("Sender after memcpy: %02X\n", shmaddr[777777]);

    SHM_info shm_info;
    shm_info.mtype = 1;
    shm_info.shm_id = shmid;

    // create msg queue
    int msg_queue_id;
    if ((msg_queue_id = msgget(SHM_MSGQ_KEY, 0666 | IPC_CREAT)) == -1)
    {
        perror("msgget error");
    }

    // send msg to queue
    if (msgsnd(msg_queue_id, &shm_info, sizeof(shm_info), 0) == -1)
    {
        perror("msgsnd error");
    }

    // Detach from shared memory
    shmdt(shmaddr);
}

// Experiment for Pipe communication
void pipes(unsigned char *image_data, int image_size)
{
    // Get the start time
    struct timespec send_time;
    if (clock_gettime(CLOCK_MONOTONIC, &send_time) < 0)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    if (do_print) printf("%ld", BILLION * send_time.tv_sec + send_time.tv_nsec);
    
    int fd;

    // Create or open the FIFO (named pipe) for writing
    if ((fd = open(PIPE_PATH, O_WRONLY)) < 0)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
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

    if (do_print) printf("%ld", BILLION * send_time.tv_sec + send_time.tv_nsec);

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