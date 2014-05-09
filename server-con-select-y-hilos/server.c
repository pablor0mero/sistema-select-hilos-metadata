/*
 * server.c
 *
 *  Created on: 08/05/2014
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <netdb.h>

#define PUERTO "8547" // Puertos que se van a escuchar: 8547 , 8548, 8549, 8550
#define MAX_PUERTOS_ESCUCHADOS 4
#define MAX_MESSAGE_SIZE 256

//Desarrollo pendiente, proximamente se recibira script de ansisop y se devolvera metadata
typedef struct {
	char mensaje[MAX_MESSAGE_SIZE];
	uint32_t mensajeSize;
}t_Paquete;

void *thread_handle_client(void*);

int recibirYDeserializar(t_Paquete*, int);


int main(int argc, char** argv) {

	struct addrinfo hints, *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	int server_sockets[MAX_PUERTOS_ESCUCHADOS];
	int maxDescriptor = -1;
	for(int i = 0; i < MAX_PUERTOS_ESCUCHADOS; i++) {

		int puerto_int = atoi(PUERTO)+i;
		char puerto_string[6];
		sprintf(puerto_string,"%d",puerto_int);

		if( getaddrinfo(NULL, puerto_string,&hints, &serverInfo) != 0) {
				puts("Error en getaddrinfo");
		}

		if ((server_sockets[i] = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)) < 0) {
			puts("Error seteando socket de server");
			return -1;
		}
		if( bind(server_sockets[i], serverInfo->ai_addr, serverInfo->ai_addrlen) < 0) {
			puts("Error bindeando socket");
			return -1;
		}
		freeaddrinfo(serverInfo);

		if( listen(server_sockets[i], 5) < 0) {
			puts("Error en el listen");
			return -1;
		}
		if(server_sockets[i] > maxDescriptor)
			maxDescriptor = server_sockets[i];
	}

	char salir = 0;
	fd_set readSockSet;


	while (!salir) {
		//Para poder estar al tanto de conexiones entrantes
		FD_ZERO(readSockSet);
		FD_SET(STDIN_FILENO,&readSockSet); //Si escribis algo en la consola del server es xq queres salir :B
		for(int i=0; i<MAX_PUERTOS_ESCUCHADOS ; i++) {
			FD_SET(server_sockets[i],&readSockSet);
		}

		int ret;
		if( (ret = select(maxDescriptor + 1, &readSockSet, NULL, NULL, NULL)) > 0) {
			if(FD_ISSET(0,&readSockSet)) {
				puts("Bye bye");
				getchar();
				salir = 1;
			}

			for(int i = 0; i < MAX_PUERTOS_ESCUCHADOS;i++ ) {
				if(FD_ISSET(server_sockets[i], readSockSet) ) {
					printf("conexion entrante en puerto %d", PUERTO+i);
					struct sockaddr_in clienteAddr;
					socklen_t clienteAddrLen = sizeof(clienteAddr);
					pthread_t threadid;
					int clienteSock = accept(server_sockets[i], (struct sockaddr*) &clienteAddr, &clienteAddrLen);
					if(clienteSock > 0) {
						pthread_create_thread(&threadid,NULL,thread_handle_client,(void *) &clienteSock);
					}
				}
			}
		}
	}
	for(int i = 0; i < MAX_PUERTOS_ESCUCHADOS; i++)
		close(server_sockets[i]);

	return 0;
}


void *thread_handle_client(void* param_cS) {

	int clientSocket = *(int *) param_cS;
	printf("Cliente %d conectado.",clientSocket);
	int bufferSize;
	char* buffer = malloc((bufferSize = sizeof(uint32_t)));

	t_Paquete paquete;
	int status = 1;

	while(status) {
		status = recibirYDeserializar(paquete,clientSocket);
		if(status) puts(paquete.mensaje);
	}
	puts("Cliente desconectado");

	close(clientSocket);

	return NULL;
}

int recibirYDeserializar(t_Paquete* paquete, int clientSocket) {
	int status;

	int buffSize;
	char* buffer = malloc(buffSize = sizeof(uint32_t));
	uint32_t mensajeSize;

	status = recv(clientSocket,buffer,sizeof(paquete->mensajeSize),0);

	memcpy(&mensajeSize,buffer,buffSize);
	if(!status) return 0;

	//Recibo el mensaje
	status = recv(clientSocket,paquete->mensaje,mensajeSize,0);
	if(!status) return 0;

	free(buffer);


	return status;
}
