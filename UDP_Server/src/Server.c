#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>


#define MAX_LENGTH 1024;

int main (int argc, char* argv[]) {



	// Open the socket
	int sd; // socket descriptor
	sd = socket (AF_INET, SOCK_DGRAM, 0); // el 0 indica que el programa decide el protocolo que quiere utilizar
	if (sd < 0) {
		perror("Socket creation");
		return 1;
	}

	// Associate socket-port
	// Edited /etc/services to include the service that we're creating (helloworld 65500/udp)
	struct sockaddr_in server;

	server.sin_family 	= AF_INET;
	server.sin_port	= htons(65500); // También se puede hacer con getservbyname (posible amplicación)
	server.sin_addr.s_addr = INADDR_ANY; /* dirección IP del cliente que atender */

	if (bind (sd, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("Bind operation");
		return 1;
	}



	// Receiving messages
	struct sockaddr_in client;

	char buffer[1024];
	int size, length;
	length = sizeof(client);


	size = recvfrom( sd, buffer, sizeof(buffer), 0, (struct sockaddr *) &client, &length);
	buffer[size] = '\0';
	printf("Client: %s\n", buffer);

	// Response to the hello from the client
	char msg [] = "Hello received";
	sendto(sd, (const char *) msg, strlen(msg), 0, (const struct sockaddr*) &client, length);

	close(sd);

	return 0;
}

