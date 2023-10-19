/*
 * Client.c
 *
 *  Created on: Oct 19, 2023
 *      Author: Sergio
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char* argv[]) {

	// Open the socket
	int sd; // socket descriptor
	sd = socket (AF_INET, SOCK_DGRAM, 0); // el 0 indica que el programa decide el protocolo que quiere utilizar


	// Associate socket-port
	// Edited /etc/services to include the service that we're creating (helloworld 65500/udp)
	struct sockaddr_in dir;
	dir.sin_family 	= AF_INET;
	dir.sin_port	= 0; // El OS elige el puerto libre que quiera
	dir.sin_addr.s_addr = INADDR_ANY; /* dirección IP del cliente que atender */


	bind (sd, (struct sockaddr *) &dir, sizeof(dir)); // el cast de sockaddr_in a sockaddr se tiene que hacer


	// Send messages through the socket
	struct sockaddr_in server;
	server.sin_family 	= AF_INET;
	server.sin_port	= htons(65500); // definido el puerto en el fichero /etc/services, tal como en el servidor

	// Se puede hacer de dos formas
	// 1.
		inet_aton("192.168.1.1", &server.sin_addr.s_addr); // con esto pasas directamente el valor a la struct (control de errores)
		// la IP se puede obtener del fichero /etc/hosts mediante la funcion gethostyname("nombre");
	// 2.
		// dir.sin_addr.s_addr = inet_addr("<INSERTAR AQUÍ LA IP>");
	// La mejor forma es la primera para el control de errores

	char msg [] = "Hello World!";


	sendto(sd, (const char *) msg, strlen(msg), 0, (struct sockaddr *) &server, sizeof(server));
	printf("Hello message sent. \n");

	// Receiving the confirm message
	int length, size;
	char buffer [1024];

	size = recvfrom(sd, (char *) buffer, 1024, 0, (struct sockaddr *) &server, &length);
	buffer[size] = '\0';
	printf("Server: %s\n", buffer);

	close(sd);

	return 0;
}
