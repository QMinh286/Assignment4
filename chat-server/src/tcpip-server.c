/*
*	FILE:					tcpip-server.c
*	ASSIGNMENT:		The "Can We Talk?" System
*	PROGRAMMERS:	Quang Minh Vu
*	DESCRIPTION:	This file holds the functions required for the chat server to function.
*/

#include "../inc/chat-server.h"
#include <time.h>

//===GLOBALS===//
static int	numClients = 0;
userInfo	userList[10];
struct 		sockaddr_in addr;
socklen_t 	addr_len;
char 		IP[INET_ADDRSTRLEN];
pthread_mutex_t userList_mutex = PTHREAD_MUTEX_INITIALIZER;

int main (void)
{
	//===VARIABLES===//
    int       server_socket, client_socket;
    int       client_len;
    struct 	  sockaddr_in client_addr, server_addr;
    int       len, i;
    pthread_t	tid[10];
    int       whichClient;
    char	  message[79];

	initializeArray();

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        return 1;
    }
  	
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_socket);
        return 1;
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        close(server_socket);
        return 2;
    }

    if (listen(server_socket, 5) < 0) 
    {
        close(server_socket);
        return 3;
    }
  
    //===MAIN LOOP===//
    while (numClients < 10) 
    {
        client_len = sizeof(client_addr);
        if ((client_socket = accept(server_socket,(struct sockaddr *)&client_addr, &client_len)) < 0) 
        {
            return 4;
        }

        if (inet_ntop(AF_INET, &client_addr.sin_addr, IP, INET_ADDRSTRLEN) == NULL) {
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&userList_mutex);
        updateArray(client_socket);
        pthread_mutex_unlock(&userList_mutex);

        if (pthread_create(&(tid[(numClients-1)]), NULL, socketThread, (void *)&client_socket))
        {
            return 5;
        }

        pthread_detach(tid[(numClients-1)]);
    }
    
    //===CHAT FULL===//
    for(i=0; i<10; i++)
    {
        pthread_join(tid[i], (void *)(&whichClient));
    }
    
    //===CLEANUP===//
    pthread_mutex_destroy(&userList_mutex);
    close(server_socket);
    return 0;
}

//==================================================FUNCTION========================|
//Name:           parcelMessage                                                     |
//Params:         char* original         The original message to be parceled.       |
//                char* parceled[]       An array of strings to store the parceled message parts.|
//                int maxParcels         The maximum number of parcels that can be created.|
//Returns:        int                        The number of parcels created.         |
//Outputs:        NONE                                                              |
//Description:    This function splits a long message into multiple parcels, ensuring each parcel is no more than 40 characters.|
//==================================================================================|
int parcelMessage(char* original, char* parceled[], int maxParcels) {
    int msgLen = strlen(original);
    int numParcels = (msgLen + 39) / 40; 
    int i, j, k = 0;

    if (numParcels > maxParcels) numParcels = maxParcels;
    
    for (i = 0; i < numParcels; i++) {
        parceled[i] = malloc(41); 
        if (parceled[i] == NULL) {
            perror("Memory allocation failed");
            for (j = 0; j < i; j++) {
                free(parceled[j]);
                parceled[j] = NULL;
            }
            return 0;
        }
        
        memset(parceled[i], 0, 41);
        
        int breakPoint = 40;
        if (i < numParcels - 1 && msgLen - k > 40) {
            for (j = 39; j >= 20; j--) {
                if (k + j < msgLen && original[k + j] == ' ') {
                    breakPoint = j + 1;
                    break;
                }
            }
        } else {
            breakPoint = (msgLen - k < 40) ? msgLen - k : 40;
        }

        strncpy(parceled[i], original + k, breakPoint);
        parceled[i][breakPoint] = '\0';
        k += breakPoint;
    }
    
    return numParcels;
}

//==================================================FUNCTION==========================================================|
//Name:					socketThread 																																													|
//Params:				void*	clientSocket	The socket for the thread to connect to.																					|
//Returns:			NONE 																																																	|
//Outputs:			NONE																																																	|
//Description:	This function represents a thread to handle the connection between the server and one of the clients.	| 
//====================================================================================================================|
void *socketThread(void *clientSocket)
{
    //===VARIABLES===//
    char buffer[BUFSIZ];
    char message[79];
    int sizeOfRead;
    int numBytesRead;
    int clSocket = *((int*)clientSocket);
    char user_ip[INET_ADDRSTRLEN];
    char userID[6] = "";
    strcpy(user_ip, IP);
    
    memset(buffer, 0, BUFSIZ);

    int iAmClient = numClients;	 

    numBytesRead = read(clSocket, buffer, BUFSIZ);
    if (numBytesRead <= 0) {
        close(clSocket);
        pthread_exit(NULL);
    }

    if (sscanf(buffer, "[%5[^]]] >>", userID) != 1) {
        strcpy(userID, "????");
    }

    while(strcmp(buffer, ">>bye<<") != 0)
    {
        if (strlen(buffer) > 0) {
            //===MESSAGE FORMAT===//
            strcpy(message, user_ip);
            strcat(message, " ");
            strcat(message, buffer);
            strcat(message, " ");

            time_t t = time(NULL);
            struct tm * time_info;
            time_info = localtime(&t);  	
            char timeChar[10];
            strftime(timeChar, sizeof(timeChar), "%H:%M:%S", time_info);
            strcat(message, timeChar);

            write(clSocket, message, strlen(message)); 

            // Parcel and broadcast message to other clients
            char* parcels[3]; // Max of 3 parcels (should be enough for 80 char limit)
            int numParcels = parcelMessage(buffer, parcels, 3);
            
            for (int i = 0; i < numParcels; i++) {
                char broadcast_message[BUFSIZ];
                
                char parsed_content[BUFSIZ];
                char *content_start = strstr(parcels[i], ">>");
                if (content_start) {
                    content_start += 3; 
                    strcpy(parsed_content, content_start);
                } else {
                    strcpy(parsed_content, parcels[i]);
                }
                
                strcpy(broadcast_message, user_ip);
                strcat(broadcast_message, " [");
                strcat(broadcast_message, userID);
                strcat(broadcast_message, "] >> ");
                strcat(broadcast_message, parsed_content);
                strcat(broadcast_message, " ");
                
                strftime(timeChar, sizeof(timeChar), "%H:%M:%S", time_info);
                strcat(broadcast_message, timeChar);
                
                writeToClients(clSocket, broadcast_message);
                free(parcels[i]);
            }
        }
     
        memset(buffer, 0, BUFSIZ);
        numBytesRead = read(clSocket, buffer, BUFSIZ);
        
        if (numBytesRead <= 0) {
            break;
        }
    }
    
    //===SHUTDOWN===//
    pthread_mutex_lock(&userList_mutex);

    for (int i = 0; i < 10; i++) {
        if (userList[i].socket == clSocket) {
            userList[i].socket = -1;
            memset(userList[i].ip, 0, 16);
            numClients--;
            break;
        }
    }
    
    pthread_mutex_unlock(&userList_mutex);
    
    close(clSocket);

    pthread_exit((void *)iAmClient);
}

//==================================================FUNCTION==============================|
//Name:					initializeArray 																													|
//Params:				NONE																																			|
//Returns:			NONE 																																			|
//Outputs:			NONE																																			|
//Description:	This function initializes the userList array to remove any garbage data.	| 
//========================================================================================|
void initializeArray(void){
	
	for(int i = 0; i < 10; i++){
		userList[i].socket = -1;
        memset(userList[i].ip, 0, 16);
	}
} 

//==================================================FUNCTION================================|
//Name:					updateArray 																																|
//Params:				int	client_socket	the socket of the client that needs updating.							|
//Returns:			NONE 																																				|
//Outputs:			NONE																																				|
//Description:	This function updates the userList array, adding socket and IP information.	| 
//==========================================================================================|
void updateArray(int client_socket){
	for(int i = 0; i < 10; i++){
		if (userList[i].socket == -1){
			userList[i].socket = client_socket;
            strcpy(userList[i].ip, IP);
			numClients++;
			break;
		}
	}
}

//==================================================FUNCTION========================|
//Name:					writeToClients 																											|
//Params:				int*	clSocket	The socket of the client that sent the message.			|
//							char	message		The message to be sent to clients.									|
//Returns:			NONE 																																|
//Outputs:			NONE																																|
//Description:	This function distributes a recieved message to all active clients.	| 
//==================================================================================|
void writeToClients(int clSocket, char message[]){
    pthread_mutex_lock(&userList_mutex);
    
    for (int i = 0; i < 10; i++){
        if(userList[i].socket != -1 && userList[i].socket != clSocket){
            int result = write(userList[i].socket, message, strlen(message));
            if (result < 0) {
                close(userList[i].socket);
                userList[i].socket = -1;
                memset(userList[i].ip, 0, 16);
                numClients--;
            }
        }
    }
    
    pthread_mutex_unlock(&userList_mutex);
}