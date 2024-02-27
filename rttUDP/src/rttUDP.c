#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
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

#define PERIOD_NS 1000000

struct timespec timespec_sub(const struct timespec *time1,
    const struct timespec *time0) {
  struct timespec diff = {
      .tv_nsec = time1->tv_nsec - time0->tv_nsec};
  if (diff.tv_nsec < 0) {
    diff.tv_nsec += 1000000000; // nsec/sec
  }
  return diff;
}
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

static void usage(void){
	printf("\nrttUDP v1.0 round-trip time UDP tester\n"
		   "Usage: rttUDP -p -n [option]*\n"
		   "-p: task priority (Real-time: 80+)\n"
		   "-n: number of packages to send (0 for unlimited)\n"
		   "-t: send task period in ms (0 no sleep time)\n"
	       "Options:\n"
	       "  SO_TIMESTAMPING - request kernel space software\n"
	       "time stamps to calculate latency\n"
	       "  CLOCK_GETTIME - request user space\n"
	       "time stamps using clock_gettime() to calculate latency \n"
	       "  ALL - request both time stamp reporting methods and calculate latency\n");
	exit(EXIT_FAILURE);
}

void* cyclicTask(void* arg) {

	//Set task period
	struct period_info pinfo;
    periodic_task_init(&pinfo);
    //Get PID
	pid_t tid = getpid();
	//Open file
    FILE *file = fopen("ts_lat.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    //Setup socket
    int sd;
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dir;
    dir.sin_family 	= AF_INET;
    dir.sin_port	= htons(35000);
    dir.sin_addr.s_addr = inet_addr("192.168.250.20");

    //Set socket options
    int priority=7;// valid values are in the range [1,7 ]1- low priority, 7 - high priority
    if (setsockopt(sd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority))  < 0) {
           perror("Failed to set SO_PRIORITY");
           exit(EXIT_FAILURE);
       }
    //Enable SO_TIMESTAMPING to generate tx and rx timestamps
    int enable =
    SOF_TIMESTAMPING_RX_SOFTWARE|SOF_TIMESTAMPING_TX_SOFTWARE
	|SOF_TIMESTAMPING_SOFTWARE|SOF_TIMESTAMPING_OPT_TSONLY;;
    if (setsockopt(sd, SOL_SOCKET, SO_TIMESTAMPING, &enable, sizeof(enable)) < 0) {
        perror("Failed to enable SO_TIMESTAMPING");
        exit(EXIT_FAILURE);
    }
    int on =1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

    //Prepare 200 bytes data to be sent
    char data[200];
    for (int i = 0; i < 200; i++) {
          data[i] = 'A';
      }


    // Main loop
    while (1) {
    	char buffer[4096]; //quitar?
		static char ctrl[1024 /* overprovision*/];
		struct msghdr msg_tx,msg_rx;
		struct iovec entry;
		struct iovec iov_rx;
		struct timespec rx_time,tx_time,rx_timestamp, tx_timestamp;
		struct timespec lat_ts, lat_time;
		struct cmsghdr *cmsg;
		struct cmsghdr *cmsg_rx;

		// Message structure
		memset(&msg_tx, 0, sizeof(msg_tx));

		entry.iov_base = data;
		entry.iov_len = sizeof(data);

		msg_tx.msg_name = NULL; //address
		msg_tx.msg_namelen = 0;//address size
		msg_tx.msg_iov = &entry; //io vector array
		msg_tx.msg_iovlen = 1; //iov length
		msg_tx.msg_control = ctrl; //ancillary data
		msg_tx.msg_controllen = sizeof(ctrl); //ancillary data lenght

		memset(&msg_rx, 0, sizeof(msg_rx));
		msg_rx.msg_name = &dir; //address
		msg_rx.msg_namelen = sizeof(dir);//address size
		msg_rx.msg_iov = &iov_rx; //io vector array
		msg_rx.msg_iovlen = 1; //iov length
		msg_rx.msg_control = buffer; //ancillary data
		msg_rx.msg_controllen = sizeof(buffer); //ancillary data lenght

        // Send 200 bytes
        clock_gettime(CLOCK_BOOTTIME ,&tx_time); //Get tx time in application space
    	if (sendto(sd, (const char *)data, sizeof(data), 0, (struct sockaddr*)&dir, sizeof(dir))<0){
           perror("sendto");
    	}
    	//Recive message with rx timestamp
    	recvmsg(sd, &msg_rx, MSG_DONTROUTE|MSG_NOSIGNAL|MSG_ZEROCOPY);
    	clock_gettime(CLOCK_BOOTTIME ,&rx_time); //Get rx time in application space
    	//Recieve tx timestamp through MSG_ERRQUEUE
        int ret = recvmsg(sd, &msg_tx, MSG_ERRQUEUE);
        if (ret == -1) {
                perror("recvmsg");
                break;
        }

        // Timespamp acquisition from kernel space
        // Extract the timestamps from ancillary data using iterator macros
        //Tx
         for (cmsg = CMSG_FIRSTHDR(&msg_tx); cmsg; cmsg = CMSG_NXTHDR(&msg_tx, cmsg)) {
              if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING) {
                  struct scm_timestamping *t = (void*)CMSG_DATA(cmsg);
                  tx_timestamp = t->ts[0]; //For hw timestamping use ts[2], ts[1] is deprecated
                  break;
              }
          }
         //Rx
         for (cmsg_rx = CMSG_FIRSTHDR(&msg_rx); cmsg_rx; cmsg_rx = CMSG_NXTHDR(&msg_rx, cmsg_rx)) {
              if (cmsg_rx->cmsg_level == SOL_SOCKET && cmsg_rx->cmsg_type == SCM_TIMESTAMPING) {
                  struct scm_timestamping *t = (void*)CMSG_DATA(cmsg_rx);
                  rx_timestamp = t->ts[0];
                  break;
              }
          }

         lat_ts=timespec_sub(&rx_timestamp,&tx_timestamp); //SO_TIMESTAMPING latency
         lat_time=timespec_sub(&rx_time,&tx_time); //clock_gettime latency

        // Calculate and print the latency

        //printf("TX_ts %ld.%ld\n",tx_timestamp.tv_sec,tx_timestamp.tv_nsec);
        //printf("RX_ts %ld.%ld\n",rx_timestamp.tv_sec,rx_timestamp.tv_nsec);

        printf("(PID: %d): Lat:%lu us. %lu\r", tid,lat_ts.tv_nsec/1000,lat_time.tv_nsec/1000);
        fprintf(file, "%lu %lu\n",lat_ts.tv_nsec/1000,lat_time.tv_nsec/1000);
        fflush(file);

        //Sleep
        wait_rest_of_period(&pinfo);
    }
    fclose(file);
    return NULL;

}


int main(int argc, char* argv[]) {
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t cyclic_thread;
    int ret;

    //Parse arguemnts
    int option;
    if(argc==1){
    	printf("Invalid arguments.\nFor detailed help run: rttUDP -h\n");
    	exit(-1);
    }
    // put ':' at the starting of the string so compiler can distinguish between '?' and ':'
    while((option = getopt(argc, argv, ":p:n:t:h")) != -1){ //get option from the getopt() method
       switch(option){
          case 'h':
        	 usage();
        	 break;
          case 'p': //Task priority (80+ for real time)
             printf("Given priority: %d\n", atoi(optarg));
             param.sched_priority = 90;
             break;
          case 'n': //Number of messages to send
             printf("Packages to send: %d\n", atoi(optarg));
             break;
          case 't': //Task period in ms
             printf("Period: %d\n", atoi(optarg));
             break;
          case 't': //Task period in ms
             printf("Period: %d\n", atoi(optarg));
             break;
          case 't': //Task period in ms
             printf("Period: %d\n", atoi(optarg));
             break;
          case 't': //Task period in ms
             printf("Period: %d\n", atoi(optarg));
             break;
          case ':':
             printf("Option needs a value\n");
             exit(-1);
             break;
          case '?': //used for some unknown options
             printf("unknown option: %c\n", optopt);
             usage();
             break;
       }
    }
    for(; optind < argc; optind++){ //when some extra arguments are passed
       printf("Given extra arguments: %s\n", argv[optind]);
    }
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
