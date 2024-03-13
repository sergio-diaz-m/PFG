/*
 * dummy.c
 *  Dummy task to isolate in a single CPU and set its irq affinity.
 *  Created on: May 13, 2024
 *      Author: Sergio Diaz
 */
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/net_tstamp.h>
#include <linux/errqueue.h>

#include <errno.h>

#define PERIOD_NS 1000000000

struct period_info {
        struct timespec next_period;
        long period_ns;
};
static void periodic_task_init(struct period_info *pinfo)
{
        /* for simplicity, hardcoding a 1ms period */
        pinfo->period_ns = PERIOD_NS;

        clock_gettime(CLOCK_REALTIME, &(pinfo->next_period));
}
static void inc_period(struct period_info *pinfo)
{
        pinfo->next_period.tv_nsec += pinfo->period_ns;

        while (pinfo->next_period.tv_nsec >= 1000000000) {
                /* timespec nsec overflow */
                pinfo->next_period.tv_sec++;
                pinfo->next_period.tv_nsec -= 1000000000;
        }
}
static void wait_rest_of_period(struct period_info *pinfo)
{
        inc_period(pinfo);

        /* for simplicity, ignoring possibilities of signal wakes */
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &pinfo->next_period, NULL);
}


void* cyclicTask(void* arg) {

	//Set task period
	struct period_info pinfo;
    periodic_task_init(&pinfo);
    struct timespec time;
    // Main loop
    while (1) {
    	clock_gettime(CLOCK_MONOTONIC,&time);
    	printf("Time: %ld.%ld\n",time.tv_sec,time.tv_nsec);
        //Sleep
        wait_rest_of_period(&pinfo);
    }
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
    param.sched_priority = 90;
    setpriority(PRIO_PROCESS, 0, 90);

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

