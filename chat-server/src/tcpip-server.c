/*
 * tcpip-server.c
 *
 * This is a sample socket server using threads
 * to requests on port 5000
 */

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

// thread function prototype
void *socketThread(void *);

// global variable to keep count of the number of clients ...
static int numClients = 0;

int main (void)
{
  int                server_socket, client_socket;
  int                client_len;
  struct sockaddr_in client_addr, server_addr;
  int                len, i;
  pthread_t	     tid[10];		//array capable of holding up to 10 "connection" threads
  int                whichClient;	

  /*
   * obtain a socket for the server
   */

  if ((server_socket = socket (AF_INET, SOCK_STREAM, 0)) < 0) 
  {
    printf ("[SERVER] : socket() FAILED\n");
    return 1;
  }
  printf("[SERVER] : socket() successful\n");
	
  /*
   * initialize our server address info for binding purposes
   */
  memset (&server_addr, 0, sizeof (server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl (INADDR_ANY);
  server_addr.sin_port = htons (PORT);

  if (bind (server_socket, (struct sockaddr *)&server_addr, sizeof (server_addr)) < 0) 
  {
    printf ("[SERVER] : bind() FAILED\n");
    close (server_socket);
    return 2;
  }
  printf ("[SERVER] : bind() successful\n");
  
  /*
   * start listening on the socket
   */
  if (listen (server_socket, 5) < 0) 
  {
    printf ("[SERVER] : listen() - FAILED.\n");
    close (server_socket);
    return 3;
  }
  printf ("[SERVER] : listen() successful\n");



  /*
   * for this server, run an endless loop that will
   * accept incoming requests from a remote client.
   * the server will create a thread to handle the
   * request, and the parent will continue to listen for the
   * next request - up to 10 clients
   */
  while (numClients < 10) 
  {
	printf("[SERVER] : Ready to accept()\n");
        fflush(stdout);	
	
       /*
	* accept a packet from the client
	*/
	client_len = sizeof (client_addr);
	if ((client_socket = accept (server_socket,(struct sockaddr *)&client_addr, &client_len)) < 0) 
	{
          printf ("[SERVER] : accept() FAILED\n");
          fflush(stdout);	
          return 4;
	}

        numClients++;
	printf("[SERVER] : received a packet from CLIENT-%02d\n", numClients);
        fflush(stdout);	

        /*
         * rather than fork and spawn the execution of the command within a 
         * child task - let's take a look at something else we could do ...
         *
         * ... we'll create a thread of execution within this task to take
         * care of executing the task.  We'll be looking at THREADING in a 
         * couple modules from now ...
         */
	if (pthread_create(&(tid[(numClients-1)]), NULL, socketThread, (void *)&client_socket))
	{
	  printf ("[SERVER] : pthread_create() FAILED\n");
          fflush(stdout);	
	  return 5;
	}

	printf("[SERVER] : pthread_create() successful for CLIENT-%02d\n", numClients);
        fflush(stdout);	


  }

  // once we reach 3 clients - let's go into a busy "join" loop waiting for
  // all of the clients to finish and join back up to this main thread
  printf("\n[SERVER] : Now we wait for the threads to complete ... \n");
  for(i=0; i<10; i++)
  {
        int joinStatus = pthread_join(tid[i], (void *)(&whichClient));
	if(joinStatus == 0)
	{
	  printf("\n[SERVER] : received >>bye<< command from CLIENT-%02d (joinStatus=%d)\n", whichClient, joinStatus);
	}
  }

  printf("\n[SERVER] : All clients have returned - exiting ...\n");
  close (server_socket);
  return 0;
}

// 
// Socket handler - this function is called (spawned as a thread of execution)
//

void *socketThread(void *clientSocket)
{
  // used for accepting incoming command and also holding the command's response
  char buffer[BUFSIZ];
  char message[BUFSIZ];
  int sizeOfRead;
  int numBytesRead;

  // remap the clientSocket value (which is a void*) back into an INT
  int clSocket = *((int*)clientSocket);

  /* Clear out the input Buffer */
  memset(buffer,0,BUFSIZ);

  // increment the numClients
  int iAmClient = numClients;	// assumes that another connection from another client 
				// hasn't been created in the meantime

  numBytesRead = read (clSocket, buffer, BUFSIZ);

  while(strcmp(buffer,">>bye<<") != 0)
  {
    /* we're actually not going to execute the command - but we could if we wanted */
    sprintf (message, "[SERVER (Thread-%02d)] : Received %d bytes - command - %s\n", iAmClient, numBytesRead, buffer);
    write (clSocket, message, strlen(message)); 

    // clear out and get the next command and process
    memset(buffer,0,BUFSIZ);
    numBytesRead = read (clSocket, buffer, BUFSIZ);
  }
  close(clSocket);
  
  // decrement the number of clients
  numClients--;

  // exit the thread and allow it to be joined in sequence
  printf ("[SERVER (Thread-%02d)] : closing socket\n", iAmClient);
  pthread_exit((void *)iAmClient);
}



