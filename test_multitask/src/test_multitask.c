/*
 * test_multitask.c
 *
 *  Created on: Nov 21, 2023
 *      Author: ubuntu
 */

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

#include "rtc.h"
#include "gpio.h"

#include <signal.h>


#define CYCLE_PERIOD_MS 100
#define BUFFER_SIZE 25

// Signal Handlers
// Global variable to indicate whether the program should exit
volatile sig_atomic_t exit_flag = 0;
void signal_handler(int signum) {
    if (signum == SIGINT) {
        // Handle Ctrl+C
        printf("Ctrl+C detected\n");
        exit_flag = 1;
    }
}
// Routines, tasks
void* udpTask(void* arg) {
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

    while (!exit_flag) {
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

        fprintf(file, "%d %d\n", timeElapsed, latency);

        // Sleep for the rest of the cycle period
        fflush(file);
        usleep(CYCLE_PERIOD_MS * 1000);
        sched_yield();
    }
    printf("Terminating UDP thread.\n");
    close(sd);
    fclose(file);
    pthread_exit(NULL);
    return NULL;
}
void* gpioTask(void *arg){
	gpio_export("21");
	gpio_set("21","out");
	while(!exit_flag){
		gpio_write("21","1");
		usleep(10000);
		gpio_write("21","0");
		usleep(10000);
		sched_yield();
	}
	printf("Terminating GPIO thread.\n");
	gpio_unexport("21");
	pthread_exit(NULL);
    return 0;

}
void* rtcTask(void* arg){
	extern int i2cFile;
	i2cFile = openI2C();
    setupRTC();
    // Set the date and time
    //setDateTime(bin2bcd(23), bin2bcd(11), bin2bcd(21), bin2bcd(14), bin2bcd(00), 0);
    // Read and print the current date and time
    while(!exit_flag){
    readDateTime();
    usleep(1000000);
    sched_yield();
    }
    printf("Terminating RTC thread.\n");
    close(i2cFile);
    pthread_exit(NULL);
    return 0;
}
// Main
int main(int argc, char* argv[]) {
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t udp_task;
    pthread_t gpio_task;
    pthread_t rtc_task;
    int ret;

    // Lock memory
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        printf("mlockall failed: %m\n");
        exit(-2);
    }
    // Set up signal handler for Ctrl+C
        if (signal(SIGINT, signal_handler) == SIG_ERR) {
            perror("Failed to set up signal handler");
            return -1;
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
    // Create threads
//    ret = pthread_create(&udp_task, &attr, udpTask, NULL);
//    if (ret) {
//        printf("create udp task pthread failed\n");
//        goto out;
//    }

    ret = pthread_create(&gpio_task, &attr, gpioTask, NULL);
    if (ret) {
        printf("create gpio task pthread failed\n");
        goto out;
    }

    ret = pthread_create(&rtc_task, &attr, rtcTask, NULL);
    if (ret) {
        printf("create rtc task pthread failed\n");
        goto out;
    }
    // Main loop
    while (!exit_flag) {
        usleep(1000);
        sched_yield();
    }

    // Join threads
//    ret = pthread_join(udp_task, NULL);
//    if (ret) {
//        printf("join udp pthread failed: %m\n");
//    }

    ret = pthread_join(gpio_task, NULL);
    if (ret) {
        printf("join gpio pthread failed: %m\n");
    }

    ret = pthread_join(rtc_task, NULL);
    if (ret) {
        printf("join rtc pthread failed: %m\n");
    }
    printf("Terminated.\n");
    out:
    return ret;

}
