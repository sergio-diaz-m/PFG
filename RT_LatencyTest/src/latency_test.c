/*
 * latency_test.c
 *
 *  Created on: Oct 24, 2023
 *      Author: ubuntu
 *
 * Measuring RTT time for an UDP communication using clock_gettime
 *
 */
//Includes
//Threads, scheduling, memory stuff
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/mman.h>
	//inet stuff
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

//Defines
#define CYCLE_PERIOD_MS 100  // Cyclic task period in ms


//CYCIC TASK TO BE DONE
void* cyclicTask (void* arg){
	struct timespec nextCycle, startTime, currentTime;
	struct sockaddr_in server;
	char msg [200];
	int latency;
	int length, size;
	char buffer[4096];
	int time;

	// Open a file for writing
	FILE *file = fopen("ts_lat.txt", "w");

	if (file == NULL) {
	    perror("Error opening file");
	    return 1;
	}
	//Preliminary task
	clock_gettime(CLOCK_MONOTONIC,&nextCycle);
	//Since no more thereads will be made or terminated pid is only checked once.
	pid_t tid = getpid();

	int sd = socket (AF_INET, SOCK_DGRAM, 0); // sd, socket descriptor, el 0 indica que el programa decide el protocolo que quiere utilizar
	struct sockaddr_in dir;
	dir.sin_family 	= AF_INET;
	dir.sin_port	= htons(35000); // El OS elige el puerto libre que quiera
	dir.sin_addr.s_addr = inet_addr("192.168.250.20"); /* dirección IP del cliente que atender */
	// Send messages through the socket
    int optval=7; // valid values are in the range [1,7]  1- low priority, 7 - high priority
    setsockopt(sd, SOL_SOCKET, 7, &optval, sizeof(optval));

	for(int i = 0; i < 200; i++){
		msg[i]= 'A';
	    }

	int num_packages=0;
	while(1){
		//Next cycle start calculation
		printf("Cyclic task (PID: %d): T:%ld s. Lat:%d us.\r", tid,nextCycle.tv_sec,latency);
        // Calculate the start time of the next cycle
//        nextCycle.tv_nsec += CYCLE_PERIOD_MS * 1000000;  // Convert ms to ns
//        while (nextCycle.tv_nsec >= 1000000000) {
//            nextCycle.tv_nsec -= 1000000000;
//            nextCycle.tv_sec++;
//        }
		clock_gettime(CLOCK_MONOTONIC,&startTime);
    	if (sendto(sd, (const char *)msg, sizeof(msg), 0 , (struct sockaddr*)&dir, sizeof(dir))<0){
            printf("Unable to send message\n");
    	}

    	// Receiving the confirm message
    	recvfrom(sd, (char *)buffer, sizeof(buffer), 0 , (struct sockaddr *)&dir, sizeof(dir));
    	clock_gettime(CLOCK_MONOTONIC,&currentTime);
    	//Get time and latency
    	latency=(currentTime.tv_nsec-startTime.tv_nsec)/1000; //microseconds
    	time=currentTime.tv_nsec; //time in nsec to be displayed.
    	fprintf(file, "%d\n",latency);

    	fflush (stdout);
		//clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&nextCycle,NULL);
	}//loop end
	fclose(file);
	return NULL;
}

int main(int argc, char* argv[])
{
        struct sched_param param;
        pthread_attr_t attr;
        pthread_t cyclic_thread;
        int ret;

        /* Lock memory */
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
                printf("mlockall failed: %m\n");
                exit(-2);
        }

        /* Initialize pthread attributes (default values) */
        ret = pthread_attr_init(&attr);
        if (ret) {
                printf("init pthread attributes failed\n");
                goto out;
        }

        /* Set a specific stack size  */
        ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
        if (ret) {
            printf("pthread setstacksize failed\n");
            goto out;
        }

        /* Set scheduler policy and priority of pthread */
        /* This offers a real time scheduling behaviour*/
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
        /* Use scheduling parameters of attr */
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret) {
                printf("pthread setinheritsched failed\n");
                goto out;
        }

        /* Create a pthread of the CYCLIC TASK  with NULL attributes */
        ret = pthread_create(&cyclic_thread, NULL , cyclicTask, NULL);
        if (ret) {
                printf("create cyclic task pthread failed\n");
                goto out;
        }

        /* Join the cyclic_thread and wait until its done*/
        ret = pthread_join(cyclic_thread, NULL);
        if (ret){
                printf("join pthread failed: %m\n");
        }
out:
        return ret;
}

