/*
 * rttUDP.c
 *  Implements latency measuring tools for UDP communications using
 *  clock_gettime and SO_TIMESTAMPING as software time stamping methods
 *  to obtain round trip time (RTT) in real-time.
 *  Created on: Oct 26, 2023
 *      Author: Sergio Diaz
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/net_tstamp.h>
#include <linux/errqueue.h>
#include <errno.h>

#define PERIOD_NS 1000000 //1ms

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
	printf("\nrttUDP v1.0 round-trip time UDP tester.\n"
		"Test output file: output.txt\n"
		"Usage: rttUDP -i-p -P -n -t -d [option]*\n"
		"-i: Destination address (Default: 192.168.250.20)\n"
		"-p: Destination port (Default: 350000)\n"
		"-P: Task priority (Real-time: 80+)\n"
		"-n: Number of packages to send (0 for unlimited)\n"
		"-t: Send task period in ms (0 no sleep time)\n"
		"-d: Debug mode (optional)\n"
		"	0: None\n"
		"	1: Print PID, latency\n"
		"	2: Print PID, latency + SO_TIMESTAMPS\n"
		"	3: Print PID, latency + SO_TIMESTAMPS\n"
			"+ send & recieve information\n"
		"Options:\n"
		"  SO_TIMESTAMPING - request kernel space software\n"
		"  time stamps to calculate latency\n"
		"  CLOCK_GETTIME - request user space\n"
		"  time stamps using clock_gettime() to calculate latency \n"
		"  ALL - request both time stamp reporting methods and calculate latency\n");
	exit(EXIT_FAILURE);
}

struct arg_data{
    char* ip;
    int port;
    int num_pkg;
    int period;
    int dbg_mode;
    char* option;
};

void* cyclicTask(void* arg) {

	//Retrieve arguemnts and cast to my structure
	struct arg_data  *args= (struct arg_data*)arg;
	int opt_flag;
	printf("Initilizing...\n");
	usleep(500000);
	printf("Send to IP: %s\n", args->ip);
	printf("Send to port: %d\n", args->port);
	if(args->num_pkg==-1){
		printf("Sending unlimited packages.\n");
	}
	else{
		printf("%d packages will be sent.\n", args->num_pkg);
	}
	printf("Send rate: %d ms\n", args->period);
	printf("Debug mode: %d\n", args->dbg_mode);
	printf("Option: %s\n", args->option);
	if (strcmp(args->option,"SO_TS")==0){
		opt_flag=1;
	}
	else if(strcmp(args->option,"CLK_GT")==0){
		opt_flag=2;
	}
	else if(strcmp(args->option,"ALL")==0){
		opt_flag=3;
	}
	else
		opt_flag=0;

	usleep(2000000);

	//Set task period
	struct period_info pinfo;
    pinfo.period_ns = PERIOD_NS*(args->period);
    periodic_task_init(&pinfo);
    //Get PID
	pid_t tid = getpid();
	//Open file
    FILE *file = fopen("output.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    //Setup socket
    int sd;
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dir;
    dir.sin_family 	= AF_INET;
    dir.sin_port	= htons(args->port);
    dir.sin_addr.s_addr = inet_addr(args->ip);

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
    int sent_pkgs=0;
    while (args->num_pkg==-1 || sent_pkgs<args->num_pkg) {
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
		if(opt_flag>=2)
			clock_gettime(CLOCK_BOOTTIME ,&tx_time); //Get tx time in application space
    	if (sendto(sd, (const char *)data, sizeof(data), 0, (struct sockaddr*)&dir, sizeof(dir))<0){
           perror("sendto");
    	}
    	if(args->dbg_mode==3){
    		printf("\nSent 200 bytes\n");
    	}
    	//Recive message with rx timestamp
    	recvmsg(sd, &msg_rx, MSG_DONTROUTE|MSG_NOSIGNAL|MSG_ZEROCOPY);
    	if(opt_flag>=2)
    		clock_gettime(CLOCK_BOOTTIME ,&rx_time); //Get rx time in application space
    	//Recieve tx timestamp through MSG_ERRQUEUE
        int ret = recvmsg(sd, &msg_tx, MSG_ERRQUEUE);
        if (ret == -1) {
                perror("recvmsg");
                break;
        }
    	if(args->dbg_mode==3){
    		printf("Recieved 200 bytes\n\n");
    	}
        // Timespamp acquisition from kernel space
        // Extract the timestamps from ancillary data using iterator macros
        //Tx
    	if(opt_flag==1||opt_flag==3){
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
    	}
         if(opt_flag==1||opt_flag==3)
        	 lat_ts=timespec_sub(&rx_timestamp,&tx_timestamp); //SO_TIMESTAMPING latency
         if(opt_flag>=2)
        	 lat_time=timespec_sub(&rx_time,&tx_time); //clock_gettime latency

        // Calculate and print the latency
        if(args->dbg_mode>=2){
        	printf("TX_ts %ld.%ld\n",tx_timestamp.tv_sec,tx_timestamp.tv_nsec);
        	printf("RX_ts %ld.%ld\n",rx_timestamp.tv_sec,rx_timestamp.tv_nsec);
        }
        if(args->dbg_mode>0){
        	printf("(PID: %d): Lat:%lu us. %lu\n", tid,lat_ts.tv_nsec/1000,lat_time.tv_nsec/1000);
        	fflush(stdout);
        }
        fprintf(file, "%lu %lu\n",lat_ts.tv_nsec/1000,lat_time.tv_nsec/1000);
        fflush(file);
        if(args->num_pkg!=-1){
        	sent_pkgs++;
        }
        //Sleep
        wait_rest_of_period(&pinfo);
    }
    printf("Finished. Sent: %d messages\n",args->num_pkg);
    fclose(file);
    return NULL;

}

int main(int argc, char* argv[]) {
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t cyclic_thread;
    int ret;
    struct arg_data th_args;
    th_args.ip="192.168.250.20"; //default for GAMESA Proteus PV GP613607PV
    th_args.port=35000; //default

    //Parse arguemnts
    int option;
    if(argc==1){
    	printf("Invalid arguments.\nFor detailed help run: rttUDP -h\n");
    	exit(-1);
    }
    // put ':' at the starting of the string so compiler can distinguish between '?' and ':'
    while((option = getopt(argc, argv, ":i:p:P:n:t:hvd:")) != -1){ //get option from the getopt() method
       switch(option){
          case 'h':
        	 usage();
        	 break;
          case 'i': //IP (default: 192.168.250.20)
              th_args.ip=optarg;
              break;
          case 'p': //Port (default: 35000)
              th_args.port=atoi(optarg);
              break;
          case 'P': //Task priority (80+ for real time)
              printf("Given priority: %d\n", atoi(optarg));
              param.sched_priority=atoi(optarg);
              break;
		   case 'n': //Number of messages to send
			  th_args.num_pkg=atoi(optarg);
			  break;
		   case 't': //Task period in ms
			  th_args.period=atoi(optarg);
			  break;
		   case 'd': //Debug mdoe
			  th_args.dbg_mode=atoi(optarg);
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
       th_args.option=argv[optind];
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
    ret = pthread_create(&cyclic_thread, NULL, cyclicTask,(void*)&th_args);
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
