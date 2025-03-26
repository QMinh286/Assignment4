#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>

#define PORT 5000

typedef struct{
	int 	*socket;
	char	ip[16];
}userInfo;

void *socketThread(void *);
void initializeArray(void);
void initializeServerAddress(struct sockaddr_in server_addr);
void updateArray(int client_socket);
void writeToClients(int* clSocket, char message[]);
