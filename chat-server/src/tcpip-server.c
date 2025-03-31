/*
 *	FILE:					tcpip-server.c
 *	ASSIGNMENT:		The "Can We Talk?" System
 *	PROGRAMMERS:	Jose Morales Gutierrez, Oliver Gingerich, Quang Minh Vu
 *	DESCRIPTION:	This file holds the functions required for the chat server to function.
 */

#include "../inc/chat-server.h"

//===GLOBALS===//
static int numClients = 0;
userInfo userList[10];
struct sockaddr_in addr;
socklen_t addr_len;
char IP[INET_ADDRSTRLEN];

int main(void)
{
  //===VARIABLES===//
  int server_socket, client_socket;
  int client_len;
  struct sockaddr_in client_addr, server_addr;
  int len, i;
  pthread_t tid[10];
  int whichClient;
  char message[79];

  initializeArray();

  if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr,"[SERVER] : socket() FAILED\n");
    return 1;
  }

  //===INITIALIZING MEMORY===//
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(PORT);

  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    fprintf(stderr,"[SERVER] : bind() FAILED\n");
    close(server_socket);
    return 2;
  }

  if (listen(server_socket, 5) < 0)
  {
    fprintf(stderr,"[SERVER] : listen() - FAILED.\n");
    close(server_socket);
    return 3;
  }

  //===MAIN LOOP===//
  while (numClients < 10)
  {
    fflush(stdout);

    client_len = sizeof(client_addr);
    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len)) < 0)
    {
      fprintf(stderr,"[SERVER] : accept() FAILED\n");
      fflush(stdout);
      return 4;
    }

    if (inet_ntop(AF_INET, &client_addr, IP, INET_ADDRSTRLEN) == NULL)
    {
      perror("inet_ntop");
      exit(EXIT_FAILURE);
    }

    updateArray(client_socket);
    fflush(stdout);

    if (pthread_create(&(tid[(numClients - 1)]), NULL, socketThread, (void *)&client_socket))
    {
      printf("[SERVER] : pthread_create() FAILED\n");
      fflush(stdout);
      return 5;
    }

    fflush(stdout);
  }
  //===CHAT FULL===//
  printf("\n[SERVER] CHAT FULL: Waiting for client to leave. \n");
  for (i = 0; i < 10; i++)
  {
    int joinStatus = pthread_join(tid[i], (void *)(&whichClient));
    if (joinStatus == 0)
    {
      printf("\n[SERVER] CHAT SPOT OPENED (joinStatus=%d)\n", whichClient, joinStatus);
    }
  }
  //===CLEANUP===//
  close(server_socket);
  return 0;
}

//==================================================FUNCTION==========================================================|
// Name:				socketThread 																																												  |
// Params:			void*	clientSocket	The socket for the thread to connect to.																				  |
// Returns:			NONE 																																																	|
// Outputs:			NONE																																																	|
// Description:	This function represents a thread to handle the connection between the server and one of the clients.	|
//====================================================================================================================|
void *socketThread(void *clientSocket)
{
  //===VARIABLES===//
  char buffer[BUFSIZ];
  char message[79];
  int sizeOfRead;
  int numBytesRead;
  int clSocket = *((int *)clientSocket);

  memset(buffer, 0, BUFSIZ);

  int iAmClient = numClients;

  numBytesRead = read(clSocket, buffer, BUFSIZ);

  while (strcmp(buffer, ">>bye<<") != 0)
  {
    //===MESSAGE FORMAT===//
    char userID[6], msg[41];
    sscanf(buffer, "[%5[^]]]%*[^>]>> %40[^\n]", userID, msg);

    // Format message: "IP [user] << msg time"
    time_t t = time(NULL);
    struct tm *time_info = localtime(&t);
    char timeChar[10];
    strftime(timeChar, sizeof(timeChar), "%H:%M:%S", time_info);

    snprintf(message, sizeof(message), "%-15s [%5s] << %-40s (%s)", IP, userID, msg, timeChar);
    writeToClients(clSocket, message);

    // Echo to sender with ">>"
    snprintf(message, sizeof(message), "%-15s [%5s] >> %-40s (%s)", IP, userID, msg, timeChar);
    write(clSocket, message, strlen(message));

    memset(buffer, 0, BUFSIZ);
    numBytesRead = read(clSocket, buffer, BUFSIZ);
  }
  //===SHUTDOWN===//
  close(clSocket);
  numClients--;

  pthread_exit((void *)iAmClient);
}

//==================================================FUNCTION==============================|
// Name:				initializeArray 																													|
// Params:			NONE																																			|
// Returns:			NONE 																																			|
// Outputs:			NONE																																			|
// Description:	This function initializes the userList array to remove any garbage data.	|
//========================================================================================|
void initializeArray(void)
{

  for (int i = 0; i < 10; i++)
  {
    userList[i].socket = -1;
  }
}

//==================================================FUNCTION================================|
// Name:				updateArray 																																|
// Params:			int	client_socket	the socket of the client that needs updating.							|
// Returns:			NONE 																																				|
// Outputs:			NONE																																				|
// Description:	This function updates the userList array, adding socket and IP information.	|
//==========================================================================================|
void updateArray(int client_socket)
{

  for (int i = 0; i < 10; i++)
  {
    if (userList[i].socket == -1)
    {
      userList[i].socket = client_socket;
      numClients++;
      break;
    }
  }
}

//==================================================FUNCTION========================|
// Name:				writeToClients 																											|
// Params:			int*	clSocket	The socket of the client that sent the message.			|
//							char	message		The message to be sent to clients.									|
// Returns:			NONE 																																|
// Outputs:			NONE																																|
// Description:	This function distributes a recieved message to all active clients.	|
//==================================================================================|
void writeToClients(int *clSocket, char message[])
{

  for (int i = 0; i < 10; i++)
  {
    if (userList[i].socket != -1 && userList[i].socket != clSocket)
    {
      write(userList[i].socket, message, strlen(message));
    }
  }
}
