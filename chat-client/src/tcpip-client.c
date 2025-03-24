/*
 * tcpip-client.c
 *
 * This is a sample internet client application that will talk
 * to the server s.c via port 5000
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

#define PORT 5000

char buffer[BUFSIZ];

int main (int argc, char *argv[])
{
  int                my_server_socket, len, done;
  struct sockaddr_in server_addr;
  struct hostent*    host;

  /*
   * check for sanity
   */
  if (argc != 2) 
  {
    printf ("USAGE : tcpipClient <server_name>\n");
    return 1;
  }

  /*
   * determine host info for server name supplied
   */
  if ((host = gethostbyname (argv[1])) == NULL) 
  {
    printf ("[CLIENT] : Host Info Search - FAILED\n");
    return 2;
  }

  /*
   * initialize struct to get a socket to host
   */
  memset (&server_addr, 0, sizeof (server_addr));
  server_addr.sin_family = AF_INET;
  memcpy (&server_addr.sin_addr, host->h_addr, host->h_length); // copy the host's internal IP addr into the server_addr struct
  server_addr.sin_port = htons (PORT);

     /*
      * get a socket for communications
      */
     printf ("[CLIENT] : Getting STREAM Socket to talk to SERVER\n");
     fflush(stdout);
     if ((my_server_socket = socket (AF_INET, SOCK_STREAM, 0)) < 0) 
     {
       printf ("[CLIENT] : Getting Client Socket - FAILED\n");
       return 3;
     }

  
     /*
      * attempt a connection to server
      */
     printf ("[CLIENT] : Connecting to SERVER\n");
     fflush(stdout);
     if (connect (my_server_socket, (struct sockaddr *)&server_addr,sizeof (server_addr)) < 0) 
     {
       printf ("[CLIENT] : Connect to Server - FAILED\n");
       close (my_server_socket);
       return 4;
     }

  done = 1;
  while(done)
  {
     /* clear out the contents of buffer (if any) */
     memset(buffer,0,BUFSIZ);

     /*
      * now that we have a connection, get a commandline from
      * the user, and fire it off to the server
      */
     printf ("Enter a command [date | who | df | <enter your own command> | quit] >>> ");
     fflush (stdout);
     fgets (buffer, sizeof (buffer), stdin);
     if (buffer[strlen (buffer) - 1] == '\n') buffer[strlen (buffer) - 1] = '\0';

     /* check if the user wants to quit */
     if(strcmp(buffer,"quit") == 0)
     {
	// send the command to the SERVER
       write (my_server_socket, buffer, strlen (buffer));
       done = 0;
     }
     else
     {
       write (my_server_socket, buffer, strlen (buffer));
       len = read (my_server_socket, buffer, sizeof (buffer));
       printf ("Result of command:\n%s\n\n", buffer);
     }

  }

     /*
      * cleanup
      */
     close (my_server_socket);

  printf ("[CLIENT] : I'm outta here !\n");

  return 0;
}



