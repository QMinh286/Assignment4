/*
 * tcpip-client.c
 *
 * This is the client application for the "Can We Talk?" chat system.
 * It connects to the chat server and provides a user interface for sending
 * and receiving messages using ncurses.
 */

#include "../inc/chat-client.h"

// Global variables
char buffer[BUFSIZ];
char clientName[6];
char message_history[MAX_LINES][BUFSIZ];
int history_count = 0;
pthread_mutex_t history_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
    int my_server_socket, len, done;
    struct sockaddr_in server_addr;
    struct hostent *host;
    char message[128];
    char userID[128];
    char serverName[128];

    // Check command-line arguments
    if (argc != 3)
    {
        printf("USAGE : %s -user<userID> -server<serverName>\n", argv[0]);
        return 1;
    }

    // Parse command line arguments
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

    // Validate arguments
    if (strlen(userID) == 0 || strlen(serverName) == 0)
    {
        printf("ERROR: The -user and -server must be provided.\n");
        return 1;
    }
    
    // Ensure userID is at most 5 characters
    strncpy(clientName, userID, sizeof(clientName) - 1);
    clientName[sizeof(clientName) - 1] = '\0';

    if (strlen(userID) > 5)
    {
        printf("ERROR: The userID must be a maximum of 5 characters.\n");
        return 1;
    }

    // Resolve host
    printf("[CLIENT] : Looking up host '%s'...\n", serverName);
    if ((host = gethostbyname(serverName)) == NULL)
    {
        printf("[CLIENT] : Host Info Search - FAILED\n");
        return 2;
    }

    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);
    server_addr.sin_port = htons(PORT);

    // Create socket
    printf("[CLIENT] : Getting STREAM Socket to talk to SERVER\n");
    fflush(stdout);
    if ((my_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("[CLIENT] : Getting Client Socket - FAILED\n");
        return 3;
    }

    // Connect to server
    printf("[CLIENT] : Connecting to SERVER\n");
    fflush(stdout);
    if (connect(my_server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("[CLIENT] : Connect to Server - FAILED\n");
        close(my_server_socket);
        return 4;
    }

    // Initialize ncurses UI
    WINDOW *chat_win;
    int chat_startx, chat_starty, chat_width, chat_height;
    int msg_startx, msg_starty, msg_width, msg_height;
    int shouldBlank = 0;
    
    initscr();           // Start curses mode
    cbreak();            // Line buffering disabled
    noecho();            // Don't echo input
    keypad(stdscr, TRUE); // Enable function keys
    init_color_pair();    // Initialize color pairs
    refresh();

    // Calculate window dimensions
    chat_height = 5;
    chat_width = COLS - 2;
    chat_startx = 1;
    chat_starty = LINES - chat_height;

    msg_height = LINES - chat_height - 1;
    msg_width = COLS;
    msg_startx = 0;
    msg_starty = 0;

    // Create the message display window
    msg_win = create_newwin(msg_height, msg_width, msg_starty, msg_startx, 1);
    scrollok(msg_win, TRUE);
    wattron(msg_win, COLOR_PAIR(1));
    mvwprintw(msg_win, 0, (msg_width - 9) / 2, " Messages ");
    wattroff(msg_win, COLOR_PAIR(1));
    wrefresh(msg_win);

    // Create the chat input window
    chat_win = create_newwin(chat_height, chat_width, chat_starty, chat_startx, 2);
    scrollok(chat_win, TRUE);
    wattron(chat_win, COLOR_PAIR(2));
    mvwprintw(chat_win, 0, (chat_width - 17) / 2, " Outgoing Message ");
    wattroff(chat_win, COLOR_PAIR(2));
    wrefresh(chat_win);

    // Create thread to handle incoming messages
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_messages, (void *)&my_server_socket) != 0) {
        endwin();
        printf("[CLIENT] : Failed to create receive thread\n");
        close(my_server_socket);
        return 5;
    }

    // Display welcome message
    char welcome[BUFSIZ];
    sprintf(welcome, "Welcome to the chat, %s! Type your message and press Enter to send.", clientName);
    add_to_history(welcome);

    // Main loop for sending messages
    done = 1;
    while (done)
    {
        // Clear out the contents of buffer
        memset(buffer, 0, BUFSIZ);

        // Get user input
        input_win(chat_win, buffer);

        // If empty message, skip sending
        if (strlen(buffer) == 0) {
            continue;
        }

        // Format message according to requirements
        strcpy(message, "[");
        strcat(message, clientName);
        strcat(message, "] >> ");
        strcat(message, buffer);

        // Check if the user wants to quit
        if (strcmp(buffer, ">>bye<<") == 0)
        {
            // Send the command to the SERVER
            write(my_server_socket, message, strlen(message));
            done = 0;
            break;  // Exit the loop immediately
        }
        else
        {
            // Send message to server
            write(my_server_socket, message, strlen(message));
        }
    }

    // Cleanup
    pthread_cancel(recv_thread);
    pthread_join(recv_thread, NULL);
    pthread_mutex_destroy(&history_mutex);
    
    destroy_win(chat_win);
    destroy_win(msg_win);
    endwin();

    close(my_server_socket);

    printf("[CLIENT] : I'm outta here!\n");

    return 0;
}

//==================================================FUNCTION========================|
//Name:           init_color_pair                                                   |
//Params:         NONE                                                              |
//Returns:        NONE                                                              |
//Outputs:        NONE                                                              |
//Description:    This function initializes the color pairs for ncurses.            |
//==================================================================================|
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

//==================================================FUNCTION========================|
//Name:           create_newwin                                                     |
//Params:         int height        The height of the window.                       |
//                int width        The width of the window.                         |
//                int starty       The starting y-coordinate of the window.         |
//                int startx       The starting x-coordinate of the window.         |
//                int color_pair    The color pair for the window.                  |
//Returns:              WINDOW*         A pointer to the newly created window.      |
//Outputs:              NONE                                                        |
//Description:    This function creates a new window with a box around it and sets the background color.|
//==================================================================================|
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
//==================================================FUNCTION========================|
//Name:           input_win                                                         |
//Params:         WINDOW *win       The window to get input from.                   |
//                char *word          The buffer to store the input.                |
//Returns:        NONE                                                              |
//Outputs:        NONE                                                              |
//Description:    This function handles taking input from the user, displays it in the window,|
//                  and enforces a maximum character limit.                         |
//==================================================================================|
void input_win(WINDOW *win, char *word)
{
    int i, ch;
    int maxrow, maxcol, row = 1, col = 0;
    const int MAX_INPUT = 80; // Maximum input length

    blankWin(win);                 /* make it a clean window */
    getmaxyx(win, maxrow, maxcol); /* get window size */
    bzero(word, BUFSIZ);
    wmove(win, 1, 1); /* position cursor at top */
    
    for (i = 0; i < MAX_INPUT; i++)
    {
        ch = wgetch(win);
        
        // Stop if Enter is pressed
        if (ch == '\n')
            break;
            
        word[i] = ch;           /* store character */
        
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
    
    // If we hit the character limit, make a sound
    if (i == MAX_INPUT) {
        beep(); // Make a sound to indicate limit reached
        
        // Discard additional characters until Enter
        while ((ch = wgetch(win)) != '\n');
    }
    
    word[i] = '\0'; // Ensure null termination
    wrefresh(win);
}
//==================================================FUNCTION========================|
//Name:           display_win                                                       |
//Params:         WINDOW *win      The window to display the message.               |
//                char *word        The message to be displayed.                    |
//                int whichRow     The row where the message will be displayed.     |
//                int shouldBlank   A flag to determine if the window should be cleared before displaying.|
//Returns:        NONE                                                              |
//Outputs:        NONE                                                              |
//Description:    This function displays a message in the given window at the specified row.|
//==================================================================================|
void display_win(WINDOW *win, char *word, int whichRow, int shouldBlank)
{
    if (shouldBlank == 1)
        blankWin(win);               /* make it a clean window */
    wmove(win, (whichRow + 1), 1); /* position cursor at appropriate row */
    wprintw(win, "%s", word);
    wrefresh(win);
}


//==================================================FUNCTION========================|
//Name:           destroy_win                                                       |
//Params:         WINDOW *win        The window to be destroyed.                    |
//Returns:        NONE                                                              |
//Outputs:        NONE                                                              |
//Description:    This function destroys the specified window.                      |
//==================================================================================|
void destroy_win(WINDOW *win)
{
    delwin(win);
}

//==================================================FUNCTION========================|
//Name:           blankWin                                                          |
//Params:         WINDOW *win        The window to be cleared.                      |
//Returns:        NONE                                                              |
//Outputs:        NONE                                                              |
//Description:    This function clears the contents of the window but keeps its border.|
//==================================================================================|
void blankWin(WINDOW *win)
{
    int i;
    int maxrow, maxcol;

    getmaxyx(win, maxrow, maxcol);
    for (i = 1; i < maxrow - 1; i++)
    {
        wmove(win, i, 1);
        wclrtoeol(win);
    }
    box(win, 0, 0); /* draw the box again */
    wrefresh(win);
}

//==================================================FUNCTION========================|
//Name:           add_to_history                                                    |
//Params:         char *message        The message to be added to the history.      |
//Returns:        NONE                                                              |
//Outputs:        NONE                                                              |
//Description:    This function adds a message to the message history and updates the display window.|
//==================================================================================|
void add_to_history(char *message)
{
    pthread_mutex_lock(&history_mutex);
    
    // Debug: print to stdout for debugging
    printf("Adding to history: %s\n", message);
    
    // If buffer is full, shift all messages up
    if (history_count == MAX_LINES) {
        for (int i = 0; i < MAX_LINES - 1; i++) {
            strcpy(message_history[i], message_history[i+1]);
        }
        strcpy(message_history[MAX_LINES-1], message);
    } else {
        strcpy(message_history[history_count], message);
        history_count++;
    }
    
    // Redraw the message window
    wclear(msg_win);
    box(msg_win, 0, 0);
    wattron(msg_win, COLOR_PAIR(1));
    mvwprintw(msg_win, 0, (COLS - 9) / 2, " Messages ");
    wattroff(msg_win, COLOR_PAIR(1));
    
    for (int i = 0; i < history_count; i++) {
        // Make sure we don't exceed window width
        int max_width = COLS - 4; // Leave room for borders
        char truncated_msg[BUFSIZ];
        strncpy(truncated_msg, message_history[i], max_width);
        truncated_msg[max_width] = '\0';
        
        // Print the message
        mvwprintw(msg_win, i+1, 1, "%s", truncated_msg);
    }
    wrefresh(msg_win);
    
    pthread_mutex_unlock(&history_mutex);
}
//==================================================FUNCTION========================|
//Name:           receive_messages                                                  |
//Params:         void *arg        The socket to receive the message from.          |
//Returns:        NONE                                                              |
//Outputs:        NONE                                                              |
//Description:    This function handles receiving messages from the server, parsing, and displaying them.|
//==================================================================================|
void *receive_messages(void *arg)
{
    int sock = *((int *)arg);
    char recv_buf[BUFSIZ];
    char formatted_msg[BUFSIZ];
    
    while (1)
    {
        memset(recv_buf, 0, BUFSIZ);
        int len = read(sock, recv_buf, BUFSIZ);
        if (len <= 0)
            break;

        // Debug output
        printf("Received message: %s\n", recv_buf);
        
        // Expected format: IP.IP.IP.IP [USER] >> message HH:MM:SS
        char ip[16] = "";
        char user[6] = "";
        char msg_content[81] = "";
        char timestamp[10] = "";
        
        // Parse IP address (first segment)
        char *ptr = recv_buf;
        char *space = strchr(ptr, ' ');
        if (!space) {
            printf("Error parsing message: no space after IP\n");
            continue;
        }
        
        int ip_len = space - ptr;
        if (ip_len >= 16) ip_len = 15;
        strncpy(ip, ptr, ip_len);
        ip[ip_len] = '\0';
        
        // Move past the space
        ptr = space + 1;
        
        // Find the user between [ and ]
        char *bracket_open = strchr(ptr, '[');
        char *bracket_close = strchr(ptr, ']');
        if (!bracket_open || !bracket_close || bracket_close <= bracket_open) {
            printf("Error parsing message: problem with user brackets\n");
            continue;
        }
        
        int user_len = bracket_close - bracket_open - 1;
        if (user_len >= 6) user_len = 5;
        strncpy(user, bracket_open + 1, user_len);
        user[user_len] = '\0';
        
        // Find the message content after >>
        char *msg_marker = strstr(bracket_close, ">>");
        if (!msg_marker) {
            printf("Error parsing message: no >> marker\n");
            continue;
        }
        
        ptr = msg_marker + 3; 
        
        // Find timestamp (last part of the message)
        char *timestamp_ptr = strrchr(recv_buf, ' ');
        if (!timestamp_ptr || timestamp_ptr <= ptr) {
            printf("Error parsing message: no timestamp\n");
            continue;
        }
        
        // Extract the timestamp
        strncpy(timestamp, timestamp_ptr + 1, 8);
        timestamp[8] = '\0';
        
        // Extract the message content
        int msg_len = timestamp_ptr - ptr;
        if (msg_len > 80) msg_len = 80;
        strncpy(msg_content, ptr, msg_len);
        msg_content[msg_len] = '\0';
        
        // Format according to requirements: IP [USER] >> message timestamp
        snprintf(formatted_msg, BUFSIZ, 
                "%-15s [%-5s] >> %-40s %s", 
                ip, user, msg_content, timestamp);
        
        // Add to message history and display
        add_to_history(formatted_msg);
    }

    pthread_exit(NULL);
}