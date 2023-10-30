/*
 * latency_test.c
 *
 *  Created on: Oct 24, 2023
 *      Author: ubuntu
 *
 * POSIX Real Time Cyclic task
 * using various pthread as RT threads
 */
//Includes
//Threads, scheduling, memory stuff
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
	//inet stuff
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

//Defines
#define CYCLE_PERIOD_MS 100  // Cyclic task period in ms
#define BUFFER_SIZE 25

// Circular buffer for data stream
float circularBuffer[BUFFER_SIZE] = {0};
int currentIndex = 0; // Index for the current element


void updateBuffer(float value) {
    circularBuffer[currentIndex] = value;
    currentIndex = (currentIndex + 1) % BUFFER_SIZE; // Wrap around if needed
}
float calculateMean() {
    float sum = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        sum += circularBuffer[i];
    }
    return sum / BUFFER_SIZE;
}
//CYCIC TASK TO BE DONE
void* cyclicTask (void* arg){
	struct timespec nextCycle, t1, t1_ans;
	struct sockaddr_in server;
	char msg [200];
	int sd; // socket descriptor
	int latency;
	int length, size;
	char buffer[4096];
	int mean;

	// Open a file for writing
	FILE *file = fopen("latency.txt", "w");

	if (file == NULL) {
	    perror("Error opening file");
	    return 1;
	}
	//Preliminary task
	clock_gettime(CLOCK_MONOTONIC,&nextCycle);
	//Since no more thereads will be made or terminated pid is only checked once.
	pid_t tid = getpid();

	sd = socket (AF_INET, SOCK_DGRAM, 0); // el 0 indica que el programa decide el protocolo que quiere utilizar
	struct sockaddr_in dir;
	dir.sin_family 	= AF_INET;
	dir.sin_port	= 0; // El OS elige el puerto libre que quiera
	dir.sin_addr.s_addr = INADDR_ANY; /* dirección IP del cliente que atender */
	bind (sd, (struct sockaddr *) &dir, sizeof(dir)); // el cast de sockaddr_in a sockaddr se tiene que hacer
	// Send messages through the socket
	server.sin_family 	= AF_INET;
	server.sin_port	= htons(35000); //Server port
	inet_aton("192.168.250.20", &server.sin_addr.s_addr); //Server address



	while(1){
		//Next cycle start calculation
		printf("Cyclic task (PID: %d): T:%ld s. Lat:%d ns. Avg:%d ns. \r", tid,nextCycle.tv_sec,latency,mean);
        // Calculate the start time of the next cycle
        nextCycle.tv_nsec += CYCLE_PERIOD_MS * 1000000;  // Convert ms to ns
        while (nextCycle.tv_nsec >= 1000000000) {
            nextCycle.tv_nsec -= 1000000000;
            nextCycle.tv_sec++;
        }



    	for(int i = 0; i < 200; i++){
    		msg[i]= 'A';
    	    }

    	sendto(sd, (const char *)msg, sizeof(msg), 0, (struct sockaddr *)&server, sizeof(server));
    	//printf("Sent: %d bytes\n", sizeof(msg));
//    	t1= clock_gettime(CLOCK_MONOTONIC,&nextCycle);
    	clock_gettime(CLOCK_MONOTONIC,&t1);
    	// Receiving the confirm message
    	size = recvfrom(sd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&server, &length);
    	clock_gettime(CLOCK_MONOTONIC,&t1_ans);
    	latency=t1_ans.tv_nsec-t1.tv_nsec;
    	updateBuffer(latency);
    	fprintf(file, "%d ",t1_ans.tv_sec);
    	fprintf(file, "%d\n",latency);
    	//mean = calculateMean();
    	//printf("Received %d bytes from the server.\n", size);
    	//printf("Received %s \n", buffer);
		//Wait for the next cycle
    	//printf("Latency: %d ns. \r", latency);

    	fflush (stdout);

		clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&nextCycle,NULL);
	}//loop end
	fclose(file);
	return NULL;
}

int main(int argc, char* argv[])
{
        struct sched_param param;
        pthread_attr_t attr;
        pthread_t ot_thread;
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

