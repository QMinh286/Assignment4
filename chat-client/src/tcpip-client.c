/*
 * tcpip-client.c
 *
 * This is a sample internet client application that will talk
 * to the server s.c via port 5000
 */

#include "../inc/chat-client.h"


char buffer[BUFSIZ];
static int line_count = 0;

int main(int argc, char *argv[])
{
  int my_server_socket, len, done;
  struct sockaddr_in server_addr;
  struct hostent *host;
  char clientName[6];
  char message[48];
  char userID[128];
  char serverName[128];

  /*
   * check for sanity
   */
  if (argc != 3)
  {
    fprintf(stderr,"USAGE : %s -user<userID> -server<serverName>\n", argv[0]);
    return 1;
  }

  for (int i = 1; i < argc; i++)
  {
    if (strncmp(argv[i], "-user", 5) == 0)
    {
      strcpy(userID, argv[i] + 5);
    }
    else if (strncmp(argv[i], "-server", 7) == 0)
    {
      strcpy(serverName, argv[i] + 7);
    }
  }

  if (strlen(userID) == 0 || strlen(serverName) == 0)
  {
    fprintf(stderr,"ERROR: The -user and -server must be provided.\n");
    return 1;
  }
  // strcpy(clientName, "Test");
  strncpy(clientName, userID, sizeof(clientName) - 1);
  clientName[sizeof(clientName) - 1] = '\0';

  if (strlen(userID) > 5)
  {
    fprintf(stderr,"ERROR: The userID must be a maximum of 5 characters.\n");
    return 1;
  }

  /*
   * determine host info for server name supplied
   */
  if ((host = gethostbyname(serverName)) == NULL)
  {
    fprintf(stderr,"[CLIENT] : Host Info Search - FAILED\n");
    return 2;
  }

  // Recieve client's IP to be used in message

  /*
   * initialize struct to get a socket to host
   */
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy(&server_addr.sin_addr, host->h_addr, host->h_length); // copy the host's internal IP addr into the server_addr struct
  server_addr.sin_port = htons(PORT);

  /*
   * get a socket for communications
   */
  fprintf(stderr,"[CLIENT] : Getting STREAM Socket to talk to SERVER\n");
  fflush(stdout);
  if ((my_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr,"[CLIENT] : Getting Client Socket - FAILED\n");
    return 3;
  }

  /*
   * attempt a connection to server
   */
  fprintf(stderr,"[CLIENT] : Connecting to SERVER\n");
  fflush(stdout);
  if (connect(my_server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    fprintf(stderr,"[CLIENT] : Connect to Server - FAILED\n");
    close(my_server_socket);
    return 4;
  }

  WINDOW *chat_win;
  int chat_startx, chat_starty, chat_width, chat_height;
  int msg_startx, msg_starty, msg_width, msg_height, i;
  int shouldBlank;
  char input_buf[MAX_INPUT + 1];

  initscr(); /* Start curses mode            */
  cbreak();
  noecho();
  init_color_pair();
  refresh();

  shouldBlank = 0;

  chat_height = 5;
  chat_width = COLS - 2;
  chat_startx = 1;
  chat_starty = LINES - chat_height;

  msg_height = MAX_LINES + 2;
  msg_width = COLS;
  msg_startx = 0;
  msg_starty = 0;

  /* create the input window */
  msg_win = create_newwin(msg_height, msg_width, msg_starty, msg_startx, 1);
  scrollok(msg_win, TRUE);

  /* create the output window */
  chat_win = create_newwin(chat_height, chat_width, chat_starty, chat_startx, 2);
  scrollok(chat_win, TRUE);

  pthread_t recv_thread;
  pthread_create(&recv_thread, NULL, receive_messages, (void *)&my_server_socket);

  done = 1;
  while (done)
  {
    /* clear out the contents of buffer (if any) */
    memset(input_buf, 0, sizeof(input_buf));

    /*
     * now that we have a connection, get a commandline from
     * the user, and fire it off to the server
     */
    // printf ("Enter a command [date | who | df | <enter your own command> | >>bye<<] >>> ");
    fflush(stdout);
    input_win(chat_win, input_buf); // TODO: Limit input to 80 characters
    // If needed - Split message
    // Format message according to requirements
    if (strcmp(input_buf, ">>bye<<") == 0)
    {
      strcpy(message, "[");
      strcat(message, clientName);
      strcat(message, "] >> ");
      strcat(message, input_buf);
      write(my_server_socket, message, strlen(message));
      done = 0;
    }
    else
    {   
    // Parcel message into 40-char chunks
    int len = strlen(input_buf);
    for (int i = 0; i < len; i += MAX_MSG)
      {
        char chunk[MAX_MSG + 1];
        int chunk_len = (len - i > MAX_MSG) ? MAX_MSG : (len - i);
        strncpy(chunk, input_buf + i, chunk_len);
        chunk[chunk_len] = '\0';

        strcpy(message, "[");
        strcat(message, clientName);
        strcat(message, "] >> ");
        strcat(message, chunk);
        write(my_server_socket, message, strlen(message));
      }
    }
  }

  /*
   * cleanup
   */

  destroy_win(chat_win);
  destroy_win(msg_win);
  endwin();

  pthread_join(recv_thread, NULL);

  close(my_server_socket);

  printf("[CLIENT] : I'm outta here !\n");

  return 0;
}
//==================================================FUNCTION==========================================================|
// Name:				init_color_pair 																																										  |
// Params:			NONE                                                        																				  |
// Returns:			NONE 																																																	|
// Outputs:			NONE																																																	|
// Description:	This function will initialize the color of the client.                                              	|
//====================================================================================================================|
void init_color_pair()
{
  if (start_color() == ERR)
  {
    endwin();
    fprintf(stderr, "Could not initialize color\n");
    exit(1);
  }
  if (has_colors() == FALSE)
  {
    endwin();
    fprintf(stderr, "Your terminal does not support color\n");
    exit(1);
  }

  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
}
//==================================================FUNCTION==========================================================|
// Name:        create_newwin                                                                                         |
// Params:      int     height       The height of the new window.                                                    |
//              int     width        The width of the new window.                                                     |
//              int     starty       The starting y-coordinate of the window.                                         |
//              int     startx       The starting x-coordinate of the window.                                         |
//              int     color_pair   The color pair to be used for the window.                                        |
// Returns:     WINDOW*              Pointer to the newly created window.                                             |
// Outputs:     NONE                                                                                           	      |
// Description: This function creates a new ncurses window with a border and background color.                        |
//====================================================================================================================|
WINDOW *create_newwin(int height, int width, int starty, int startx, int color_pair)
{
  WINDOW *local_win;

  local_win = newwin(height, width, starty, startx);
  wbkgd(local_win, COLOR_PAIR(color_pair));
  box(local_win, 0, 0);   /* draw a box */
  wmove(local_win, 1, 1); /* position cursor at top */
  wrefresh(local_win);
  return local_win;
}

//==================================================FUNCTION==========================================================|
// Name:        input_win                                                                                             |
// Params:      WINDOW*  win         The ncurses window where input is received.                                      |
//              char*    word        The buffer to store user input.                                                  |
// Returns:     NONE                                                                                                  |
// Outputs:     The user input is displayed in the specified ncurses window.                                          |
// Description: This function takes input from the user character by character and displays it in the window.         |
//====================================================================================================================|
/* This function is for taking input chars from the user */
void input_win(WINDOW *win, char *word)
{
  int i, ch;
  int maxrow, maxcol, row = 1, col = 0;

  blankWin(win);                 /* make it a clean window */
  getmaxyx(win, maxrow, maxcol); /* get window size */
  bzero(word, BUFSIZ);
  wmove(win, 1, 1); /* position cusor at top */
  for (i = 0; (ch = wgetch(win)) != '\n'; i++)
  {
    word[i] = ch;           /* '\n' not copied */
    if (col++ < maxcol - 2) /* if within window */
    {
      wprintw(win, "%c", word[i]); /* display the char recv'd */
    }
    else /* last char pos reached */
    {
      col = 1;
      if (row == maxrow - 2) /* last line in the window */
      {
        scroll(win);          /* go up one line */
        row = maxrow - 2;     /* stay at the last line */
        wmove(win, row, col); /* move cursor to the beginning */
        wclrtoeol(win);       /* clear from cursor to eol */
        box(win, 0, 0);       /* draw the box again */
      }
      else
      {
        row++;
        wmove(win, row, col); /* move cursor to the beginning */
        wrefresh(win);
        wprintw(win, "%c", word[i]); /* display the char recv'd */
      }
    }
  }
} /* input_win */
//==================================================FUNCTION==========================================================|
// Name:        display_win                                                                                           |
// Params:      WINDOW*  win          The ncurses window where the message is displayed.                              |
//              char*    word         The message to be displayed.                                                    |
//              int      whichRow     The row in the window where the message should appear.                          |
//              int      shouldBlank  Whether to clear the window before displaying (1 = clear, 0 = append).          |
// Returns:     NONE                                                                                                  |
// Outputs:     The provided message is displayed in the specified window.                                            |
// Description: This function displays a message in a specified row of the ncurses window.                            |
//====================================================================================================================|
void display_win(WINDOW *win, char *word, int whichRow, int shouldBlank)
{
  if (shouldBlank == 1)
    blankWin(win);               /* make it a clean window */
  if (line_count >= MAX_LINES)
  {
    wscrl(win, 1); // Scroll up one line
    wmove(win, MAX_LINES, 1);
  }
  else
  {
    wmove(win, line_count + 1, 1);
    line_count++;
  }
  wmove(win, (whichRow + 1), 1); /* position cusor at approp row */
  wprintw(win, word);
  wrefresh(win);
} /* display_win */

void destroy_win(WINDOW *win)
{
  delwin(win);
} /* destory_win */

void blankWin(WINDOW *win)
{
  int i;
  int maxrow, maxcol;

  getmaxyx(win, maxrow, maxcol);
  for (i = 1; i < maxcol - 2; i++)
  {
    wmove(win, i, 1);
    refresh();
    wclrtoeol(win);
    wrefresh(win);
  }
  box(win, 0, 0); /* draw the box again */
  wrefresh(win);
} /* blankWin */

void *receive_messages(void *arg)
{
  int sock = *((int *)arg);
  char recv_buf[BUFSIZ];

  while (1)
  {
    memset(recv_buf, 0, BUFSIZ);
    int len = read(sock, recv_buf, BUFSIZ);
    if (len <= 0)
      break;

    display_win(msg_win, recv_buf, 10, 0);
  }

  pthread_exit(NULL);
}
