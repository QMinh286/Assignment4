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
#include <stdarg.h>
#include <fcntl.h>
#include <ncurses.h>

#define PORT 5000

WINDOW *create_newwin(int, int, int, int,int);
void destroy_win(WINDOW *);
void input_win(WINDOW *, char *);
void display_win(WINDOW *, char *, int, int);
void destroy_win(WINDOW *win);
void blankWin(WINDOW *win);
void init_color_pair();

char buffer[BUFSIZ];

int main (int argc, char *argv[])
{
  int                my_server_socket, len, done;
  struct sockaddr_in server_addr;
  struct hostent*    host;
  char clientName[6];
  char message[48];

  strcpy(clientName, "Test");

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

  //Recieve client's IP to be used in message

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

  WINDOW *chat_win, *msg_win;
  int chat_startx, chat_starty, chat_width, chat_height;
  int msg_startx, msg_starty, msg_width, msg_height, i;
  int shouldBlank;
  char buf[BUFSIZ];

  initscr();                      /* Start curses mode            */
  cbreak();
  noecho();
  init_color_pair();
  refresh();
  
  shouldBlank = 0;

  chat_height = 5;
  chat_width  = COLS - 2;
  chat_startx = 1;
  chat_starty = LINES - chat_height;

  msg_height = LINES - chat_height - 1;
  msg_width  = COLS;
  msg_startx = 0;
  msg_starty = 0;

  /* create the input window */
  msg_win = create_newwin(msg_height, msg_width, msg_starty, msg_startx,1);
  scrollok(msg_win, TRUE);

  /* create the output window */
  chat_win = create_newwin(chat_height, chat_width, chat_starty, chat_startx,2);
  scrollok(chat_win, TRUE);


  done = 1;
  while(done)
  {
     /* clear out the contents of buffer (if any) */
     memset(buffer,0,BUFSIZ);

     /*
      * now that we have a connection, get a commandline from
      * the user, and fire it off to the server
      */
    // printf ("Enter a command [date | who | df | <enter your own command> | >>bye<<] >>> ");
     fflush (stdout);
     input_win(chat_win, buffer);//TODO: Limit input to 80 characters
     //If needed - Split message
     //Format message according to requirements
     strcpy(message, "[");
     strcat(message, clientName);
     strcat(message, "] >> ");
     strcat(message, buffer);

     /* check if the user wants to quit */
     if(strcmp(buffer,">>bye<<") == 0)
     {
	// send the command to the SERVER
       write (my_server_socket, message, strlen (message));
       done = 0;
     }
     else
     {
       write (my_server_socket, message, strlen (message));
       len = read (my_server_socket, buffer, sizeof (buffer));
       display_win(msg_win, buffer, 10, shouldBlank);
     }

  }

     /*
      * cleanup
      */

  destroy_win(chat_win);
  destroy_win(msg_win);
  endwin();

  close (my_server_socket);

  printf ("[CLIENT] : I'm outta here !\n");

  return 0;
}

void init_color_pair(){
  if(start_color() ==ERR){
	endwin();
	fprintf(stderr,"Could not initialize color\n");
	exit(1);
  }
  if(has_colors() ==FALSE){
 	endwin();
	fprintf(stderr,"Your terminal does not support color\n");
	exit(1);
  }
  
  init_pair(1,COLOR_RED,COLOR_BLACK);
  init_pair(2,COLOR_GREEN,COLOR_BLACK);
}
WINDOW *create_newwin(int height, int width, int starty, int startx,int color_pair)
{
  WINDOW *local_win;

  local_win = newwin(height, width, starty, startx);
  wbkgd(local_win,COLOR_PAIR(color_pair));
  box(local_win, 0, 0);               /* draw a box */
  wmove(local_win, 1, 1);             /* position cursor at top */
  wrefresh(local_win);
  return local_win;
}

/* This function is for taking input chars from the user */
void input_win(WINDOW *win, char *word)
{
  int i, ch;
  int maxrow, maxcol, row = 1, col = 0;

  blankWin(win);                          /* make it a clean window */
  getmaxyx(win, maxrow, maxcol);          /* get window size */
  bzero(word, BUFSIZ);
  wmove(win, 1, 1);                       /* position cusor at top */
  for (i = 0; (ch = wgetch(win)) != '\n'; i++)
  {
    word[i] = ch;                       /* '\n' not copied */
    if (col++ < maxcol-2)               /* if within window */
    {
      wprintw(win, "%c", word[i]);      /* display the char recv'd */
    }
    else                                /* last char pos reached */
    {
      col = 1;
      if (row == maxrow-2)              /* last line in the window */
      {
        scroll(win);                    /* go up one line */
        row = maxrow-2;                 /* stay at the last line */
        wmove(win, row, col);           /* move cursor to the beginning */
        wclrtoeol(win);                 /* clear from cursor to eol */
        box(win, 0, 0);                 /* draw the box again */
      }
      else
      {
        row++;
        wmove(win, row, col);           /* move cursor to the beginning */
        wrefresh(win);
        wprintw(win, "%c", word[i]);    /* display the char recv'd */
      }
    }
  }
}  /* input_win */

void display_win(WINDOW *win, char *word, int whichRow, int shouldBlank)
{
  if(shouldBlank == 1) blankWin(win);                /* make it a clean window */
  wmove(win, (whichRow+1), 1);                       /* position cusor at approp row */
  wprintw(win, word);
  wrefresh(win);
} /* display_win */

void destroy_win(WINDOW *win)
{
  delwin(win);
}  /* destory_win */

void blankWin(WINDOW *win)
{
  int i;
  int maxrow, maxcol;

  getmaxyx(win, maxrow, maxcol);
  for (i = 1; i < maxcol-2; i++)
  {
    wmove(win, i, 1);
    refresh();
    wclrtoeol(win);
    wrefresh(win);
  }
  box(win, 0, 0);             /* draw the box again */
  wrefresh(win);
}  /* blankWin */

