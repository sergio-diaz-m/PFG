# UDP round-trip time pinger
This networking tool has been designed to measure the latency within a network that consists in a UDP client and a UDP server which replies the data sent.

## How to run
After setting up the UDP server that will reply the 200 bytes sent start the client using:

```shell
> ./rttUDP -i [IP_ADDR] -p [PORT] -P [PRIO] -n -t -f -d [OPTION]
```
Where each parameter represents:
- `-i [IP_ADDR]` Destination address (Default: 192.168.250.20)
- - `-p [PORT]` Destination port (Default: 350000)
- `-P [PRIO]` Task priority (Real-time: 80+)
- `-n`: Number of packages to send (-1 for unlimited)
- `-t`: Send task period in ms (Default: 1ms)
- `-f`: [filename] output text file name (Default: output.txt)
- `-d`: Debug mode: (optional)
  	0: None.
	1: Print PID, latency.
  	2: Print PID, latency + SO_TIMESTAMPS.
  	3: Print PID, latency + SO_TIMESTAMPS + send & recieve information.
  -`[OPTIONS]`
  	SO_TIMESTAMPING - request kernel space software time stamps to calculate latency.
  	CLOCK_GETTIME - request user space time stamps using clock_gettime() to calculate latency.
  	ALL - request both time stamp reporting methods and calculate latency.
      
