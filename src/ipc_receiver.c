#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#define PIPE_PATH "pipe"
#define BILLION 1000000000L  // 1 billion nanoseconds in a second

// Function declarations
void shared_memory();
void pipes();
void msg_queue();

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <function_name>\n", argv[0]);
        return 1;
    }

    // Check the string argument and call respective functions
    if (strcmp(argv[1], "shared") == 0) {
        shared_memory();
    } else if (strcmp(argv[1], "pipes") == 0) {
        pipes();
    } else if (strcmp(argv[1], "queue") == 0) {
        msg_queue();
    } else {
        printf("Invalid function name: %s. Valid names are 'shared', 'pipes' and 'queue'\n", argv[1]);
        return 1;
    }

    return 0;
}

// Experiment for Shared Memory
void shared_memory() {
    printf("This is function A.\n");
}

// Experiment for Pipe communication
void pipes() {
    char buffer[100];
    int fd;
    struct timespec receive_time;

    // Create or open the FIFO (named pipe) for reading
    if ((fd = open(PIPE_PATH, O_RDONLY)) < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Read message from the FIFO
    read(fd, buffer, sizeof(buffer));

    // Get the end time
    if (clock_gettime(CLOCK_MONOTONIC, &receive_time) < 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    printf("%ld", BILLION * receive_time.tv_sec + receive_time.tv_nsec);
    // printf("Recieve time was: %llu \n", receive_time.tv_nsec);

    // printf("Receiver received message: %s\n", buffer);

    // Close the FIFO
    close(fd);
}

// Experiment for message queue communication
void msg_queue() {
    printf("This is function C.\n");
}