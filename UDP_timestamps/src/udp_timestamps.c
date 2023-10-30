#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define CYCLE_PERIOD_MS 100
#define BUFFER_SIZE 25

void* cyclicTask(void* arg) {
    FILE *file = fopen("ts_lat.txt", "w");

    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    int sd;
    struct sockaddr_in server;
    char msg[200];
    int latency;

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dir;
    dir.sin_family = AF_INET;
    dir.sin_port = 0;
    dir.sin_addr.s_addr = INADDR_ANY;
    bind(sd, (struct sockaddr*)&dir, sizeof(dir));

    // Enable the SO_TIMESTAMP option for the socket
    int timestampOption = 1;
    setsockopt(sd, SOL_SOCKET, SO_TIMESTAMP, &timestampOption, sizeof(timestampOption));

    server.sin_family = AF_INET;
    server.sin_port = htons(35000); // Server port
    inet_aton("192.168.250.20", &server.sin_addr.s_addr); // Server address

    struct timeval startTime, currentTime;
    gettimeofday(&startTime, NULL);

    while (1) {
        for (int i = 0; i < 200; i++) {
            msg[i] = 'A';
        }

        struct timeval sendTime, receiveTime;

        // Record the send time
        gettimeofday(&sendTime, NULL);

        sendto(sd, (const char*)msg, sizeof(msg), 0, (struct sockaddr*)&server, sizeof(server));

        // Receive the confirmation message along with the timestamp
        struct msghdr msg;
        struct iovec iov;
        char buffer[4096];
        int size;
        int length;

        iov.iov_base = buffer;
        iov.iov_len = sizeof(buffer);
        msg.msg_name = &server;
        msg.msg_namelen = sizeof(server);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        // Set up a control message to receive the timestamp
        char control[CMSG_SPACE(sizeof(struct timeval))];
        msg.msg_control = control;
        msg.msg_controllen = sizeof(control);

        size = recvmsg(sd, &msg, 0);

        // Extract the timestamp from the control message
        struct cmsghdr *cmsg;
        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMP) {
                struct timeval *timestamp = (struct timeval *)CMSG_DATA(cmsg);
                receiveTime = *timestamp;
            }
        }

        // Calculate and print the latency
        timersub(&receiveTime, &sendTime, &receiveTime);
        latency = receiveTime.tv_sec * 1000000 + receiveTime.tv_usec;

        // Calculate time elapsed since the start of the program
        gettimeofday(&currentTime, NULL);
        timersub(&currentTime, &startTime, &currentTime);
        int timeElapsed = currentTime.tv_sec * 1000000 + currentTime.tv_usec;

        printf("Latency: %d microseconds, Time Elapsed: %d microseconds\n", latency, timeElapsed);

        fprintf(file, "Time Elapsed: %d microseconds, Latency: %d microseconds\n", timeElapsed, latency);

        // Sleep for the rest of the cycle period
        fflush(file);
        usleep(CYCLE_PERIOD_MS * 1000);
    }
    fclose(file);
    return NULL;
}


int main(int argc, char* argv[]) {
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t cyclic_thread;
    int ret;

    // Lock memory
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }

    // Initialize pthread attributes (default values)
    ret = pthread_attr_init(&attr);
    if (ret) {
        printf("init pthread attributes failed\n");
        goto out;
    }

    // Set a specific stack size
    ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
    if (ret) {
        printf("pthread setstacksize failed\n");
        goto out;
    }

    // Set scheduler policy and priority of pthread
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret) {
        printf("pthread setschedpolicy failed\n");
        goto out;
    }
    param.sched_priority = 80;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret) {
        printf("pthread setschedparam failed\n");
        goto out;
    }

    // Use scheduling parameters of attr
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret) {
        printf("pthread setinheritsched failed\n");
        goto out;
    }

    // Create a pthread of the CYCLIC TASK with NULL attributes
    ret = pthread_create(&cyclic_thread, NULL, cyclicTask, NULL);
    if (ret) {
        printf("create cyclic task pthread failed\n");
        goto out;
    }

    // Join the cyclic_thread and wait until it's done
    ret = pthread_join(cyclic_thread, NULL);
    if (ret) {
        printf("join pthread failed: %m\n");
    }

out:
    return ret;
}
