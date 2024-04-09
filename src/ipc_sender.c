#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#define PIPE_PATH "pipes"
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
    char message[] = "Hello from sender!";
    int fd;
    struct timespec send_time;

    // Create or open the FIFO (named pipe) for writing
    if ((fd = open(PIPE_PATH, O_WRONLY)) < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Get the start time
    if (clock_gettime(CLOCK_MONOTONIC, &send_time) < 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    
    printf("%ld", BILLION * send_time.tv_sec + send_time.tv_nsec);
    // printf("Send time was: %llu \n", send_time.tv_nsec);

    // Write message to the FIFO
    write(fd, message, strlen(message) + 1);
    // printf("Sender sent message: %s\n", message);

    // Close the FIFO
    close(fd);
}

// Experiment for message queue communication
void msg_queue() {
    printf("This is function C.\n");
}