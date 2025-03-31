/*
* FILE: tcpip-server.c
* ASSIGNMENT: The "Can We Talk?" System
* PROGRAMMERS: Quang Minh Vu
* DESCRIPTION: It connects to the chat server and provides a user interface for sending
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

    if (argc != 3)
    {
        printf("USAGE : %s -user<userID> -server<serverName>\n", argv[0]);
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
        printf("ERROR: The -user and -server must be provided.\n");
        return 1;
    }

    strncpy(clientName, userID, sizeof(clientName) - 1);
    clientName[sizeof(clientName) - 1] = '\0';

    if (strlen(userID) > 5)
    {
        printf("ERROR: The userID must be a maximum of 5 characters.\n");
        return 1;
    }

    if ((host = gethostbyname(serverName)) == NULL)
    {
        printf("ERROR: Host not found.\n");
        return 2;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);
    server_addr.sin_port = htons(PORT);

    if ((my_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("ERROR: Could not create socket.\n");
        return 3;
    }

    if (connect(my_server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("ERROR: Could not connect to server.\n");
        close(my_server_socket);
        return 4;
    }

    WINDOW *chat_win;
    int chat_startx, chat_starty, chat_width, chat_height;
    int msg_startx, msg_starty, msg_width, msg_height;
    int shouldBlank = 0;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    init_color_pair();
    refresh();

    chat_height = 5;
    chat_width = COLS - 2;
    chat_startx = 1;
    chat_starty = LINES - chat_height;

    msg_height = LINES - chat_height - 1;
    msg_width = COLS;
    msg_startx = 0;
    msg_starty = 0;

    msg_win = create_newwin(msg_height, msg_width, msg_starty, msg_startx, 1);
    scrollok(msg_win, TRUE);
    wattron(msg_win, COLOR_PAIR(1));
    mvwprintw(msg_win, 0, (msg_width - 9) / 2, " Messages ");
    wattroff(msg_win, COLOR_PAIR(1));
    wrefresh(msg_win);

    chat_win = create_newwin(chat_height, chat_width, chat_starty, chat_startx, 2);
    scrollok(chat_win, TRUE);
    wattron(chat_win, COLOR_PAIR(2));
    mvwprintw(chat_win, 0, (chat_width - 17) / 2, " Outgoing Message ");
    wattroff(chat_win, COLOR_PAIR(2));
    wrefresh(chat_win);

    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_messages, (void *)&my_server_socket) != 0) {
        endwin();
        printf("ERROR: Failed to create receive thread.\n");
        close(my_server_socket);
        return 5;
    }

    char welcome[BUFSIZ];
    sprintf(welcome, "Welcome to the chat, %s! Type your message and press Enter to send.", clientName);
    add_to_history(welcome);

    done = 1;
    while (done)
    {
        memset(buffer, 0, BUFSIZ);
        input_win(chat_win, buffer);
        
        if (strlen(buffer) == 0) {
            continue;
        }
        
        strcpy(message, "[");
        strcat(message, clientName);
        strcat(message, "] >> ");
        strcat(message, buffer);
        
        if (strcmp(buffer, ">>bye<<") == 0)
        {
            write(my_server_socket, message, strlen(message));
            done = 0;
            break;
        }
        else
        {
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

    return 0;
}

//==================================================FUNCTION========================|
//Name: init_color_pair |
//Params: NONE |
//Returns: NONE |
//Outputs: NONE |
//Description: This function initializes the color pairs for ncurses. |
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
//Name: create_newwin |
//Params: int height The height of the window. |
// int width The width of the window. |
// int starty The starting y-coordinate of the window. |
// int startx The starting x-coordinate of the window. |
// int color_pair The color pair for the window. |
//Returns: WINDOW* A pointer to the newly created window. |
//Outputs: NONE |
//Description: This function creates a new window with a box around it and sets the background color.|
//==================================================================================|
WINDOW *create_newwin(int height, int width, int starty, int startx, int color_pair)
{
    WINDOW *local_win;
    local_win = newwin(height, width, starty, startx);
    wbkgd(local_win, COLOR_PAIR(color_pair));
    box(local_win, 0, 0);
    wmove(local_win, 1, 1);
    wrefresh(local_win);
    return local_win;
}

//==================================================FUNCTION========================|
//Name: input_win |
//Params: WINDOW *win The window to get input from. |
// char *word The buffer to store the input. |
//Returns: NONE |
//Outputs: NONE |
//Description: This function handles taking input from the user, displays it in the window,|
// and enforces a maximum character limit. |
//==================================================================================|
void input_win(WINDOW *win, char *word)
{
    int i, ch;
    int maxrow, maxcol, row = 1, col = 0;
    const int MAX_INPUT = 80;

    blankWin(win);
    getmaxyx(win, maxrow, maxcol);
    bzero(word, BUFSIZ);
    wmove(win, 1, 1);

    for (i = 0; i < MAX_INPUT; i++)
    {
        ch = wgetch(win);
        if (ch == '\n')
            break;
        word[i] = ch;
        if (col++ < maxcol - 2)
        {
            wprintw(win, "%c", word[i]);
        }
        else
        {
            col = 1;
            if (row == maxrow - 2)
            {
                scroll(win);
                row = maxrow - 2;
                wmove(win, row, col);
                wclrtoeol(win);
                box(win, 0, 0);
            }
            else
            {
                row++;
                wmove(win, row, col);
                wrefresh(win);
                wprintw(win, "%c", word[i]);
            }
        }
    }

    if (i == MAX_INPUT) {
        beep();
        while ((ch = wgetch(win)) != '\n');
    }

    word[i] = '\0';
    wrefresh(win);
}

//==================================================FUNCTION========================|
//Name: display_win |
//Params: WINDOW *win The window to display the message. |
// char *word The message to be displayed. |
// int whichRow The row where the message will be displayed. |
// int shouldBlank A flag to determine if the window should be cleared before displaying.|
//Returns: NONE |
//Outputs: NONE |
//Description: This function displays a message in the given window at the specified row.|
//==================================================================================|
void display_win(WINDOW *win, char *word, int whichRow, int shouldBlank)
{
    if (shouldBlank == 1)
        blankWin(win); /* make it a clean window */
    wmove(win, (whichRow + 1), 1); /* position cursor at appropriate row */
    wprintw(win, "%s", word);
    wrefresh(win);
}

//==================================================FUNCTION========================|
//Name: destroy_win |
//Params: WINDOW *win The window to be destroyed. |
//Returns: NONE |
//Outputs: NONE |
//Description: This function destroys the specified window. |
//==================================================================================|
void destroy_win(WINDOW *win)
{
    delwin(win);
}

//==================================================FUNCTION========================|
//Name: blankWin |
//Params: WINDOW *win The window to be cleared. |
//Returns: NONE |
//Outputs: NONE |
//Description: This function clears the contents of the window but keeps its border.|
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
//Name: add_to_history |
//Params: char *message The message to be added to the history. |
//Returns: NONE |
//Outputs: NONE |
//Description: This function adds a message to the message history and updates the display window.|
//==================================================================================|
void add_to_history(char *message)
{
    pthread_mutex_lock(&history_mutex);

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
//Name: receive_messages |
//Params: void *arg The socket to receive the message from. |
//Returns: NONE |
//Outputs: NONE |
//Description: This function handles receiving messages from the server, parsing, and displaying them.|
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
        
        char ip[16] = "";
        char user[6] = "";
        char msg_content[81] = "";
        char timestamp[10] = "";
        
        char *ptr = recv_buf;
        char *space = strchr(ptr, ' ');
        
        if (!space) {
            continue;
        }
        
        int ip_len = space - ptr;
        if (ip_len >= 16) ip_len = 15;
        strncpy(ip, ptr, ip_len);
        ip[ip_len] = '\0';
        
        ptr = space + 1;
        char *bracket_open = strchr(ptr, '[');
        char *bracket_close = strchr(ptr, ']');
        
        if (!bracket_open || !bracket_close || bracket_close <= bracket_open) {
            continue;
        }
        
        int user_len = bracket_close - bracket_open - 1;
        if (user_len >= 6) user_len = 5;
        strncpy(user, bracket_open + 1, user_len);
        user[user_len] = '\0';
        
        char *msg_marker = strstr(bracket_close, ">>");
        if (!msg_marker) {
            continue;
        }
        
        ptr = msg_marker + 3;
        char *timestamp_ptr = strrchr(recv_buf, ' ');
        
        if (!timestamp_ptr || timestamp_ptr <= ptr) {
            continue;
        }
        
        strncpy(timestamp, timestamp_ptr + 1, 8);
        timestamp[8] = '\0';
        
        int msg_len = timestamp_ptr - ptr;
        if (msg_len > 80) msg_len = 80;
        strncpy(msg_content, ptr, msg_len);
        msg_content[msg_len] = '\0';
        
        snprintf(formatted_msg, BUFSIZ,
                "%-15s [%-5s] >> %-40s %s",
                ip, user, msg_content, timestamp);
        
        add_to_history(formatted_msg);
    }
    
    pthread_exit(NULL);
}