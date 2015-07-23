/*
This code is a highly modified version of code written in
"TCP/IP Sockets in C : Practical guide for Programmers"
written by Michael J. Donahoo and Kenneth L. Calvert
*/

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "protocol.h"
#include "md5.h"

void DieWithError(char *errorMessage);  /* Error handling function */

int main(int argc, char *argv[])
{
    int sock;                           /* Socket descriptor */
    struct sockaddr_in servAddr; 	/* server address */
    unsigned short servPort;     	/* server port */
    char *servIP;                       /* Server IP address (dotted quad) */
    char *username, *password;          /* Account credentials */ 
    unsigned int usernameLen, passLen;  /* Length of credentials */        
    char rcvBuffer[MSGBUFSIZE+1];         /* Buffer for received string */
    char sndBuffer[MSGBUFSIZE+1];         /* Buffer for sending to server */
    int bytesSent, totalBytesSent;      /* Bytes sent in a single send() and total bytes sent */
    int bytesRcvd, totalBytesRcvd;      /* Bytes read in single recv() and total bytes read */
    char hashBuffer[HASHBUFSIZE];	/* for concatenation string, then resulting hash */
    int i; 			        /* loop indexer */
    
    if (argc != 5)    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> <Server Port> <Username> <Password>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */
    servPort = atoi(argv[2]);         /* Second arg: string to echo */
    username = argv[3];			//Third arg: username to authenticate
    usernameLen = strlen(username);     //length of username
    password = argv[4];			//Fourth arg: password to authenticate
    passLen = strlen(password);		//length of password
    
    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&servAddr, 0, sizeof(servAddr));         /* Zero out structure */
    servAddr.sin_family      = AF_INET;             /* Internet address family */
    servAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    servAddr.sin_port        = htons(servPort);     /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) 
       DieWithError("connect() failed");

    puts("\n"); //print some space
    
    //Send "Requesting Authentication" to the server
    totalBytesSent = 0;
    while (totalBytesSent < strlen(AUTH_REQUEST)) //Keep sending until entire message is sent
    {
    	//increment the pointer/decrement the size is message is fragmented
        if ((bytesSent = send(sock, AUTH_REQUEST + totalBytesSent,
	    			    strlen(AUTH_REQUEST) - totalBytesSent, 0)) <= 0)
            DieWithError("send() failed or connection closed prematurely");
        totalBytesSent += bytesSent;   /* Keep tally of total bytes */
    }
    
    printf("Sent: \"%s\"\n\n", AUTH_REQUEST);

    //Receive a Random, lower-case 64 character string
    totalBytesRcvd = 0;
    while (totalBytesRcvd < 64) //keep receiving until we have the whole string
    {
    	//increment the pointer/decrement the size is message is fragmented
        if ((bytesRcvd = recv(sock, rcvBuffer+totalBytesRcvd, 
        MSGBUFSIZE - 1 - totalBytesRcvd, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely");
        totalBytesRcvd += bytesRcvd; 
    }
    rcvBuffer[totalBytesRcvd] = '\0';  
    printf("Received Message: \"%s\"\n\n", rcvBuffer); 	
  
    //concatenate the username, password, and 64 char string
    sprintf(hashBuffer, "%s", username);
    sprintf(hashBuffer+usernameLen, "%s", password);
    sprintf(hashBuffer+usernameLen+passLen, "%s", rcvBuffer);
    printf("String to be hashed: \"%s\"\n\n", hashBuffer);
    
    //md5Hash the concatenated string
    md5_byte_t* string =  (md5_byte_t*) hashBuffer;
    md5_state_t state;
    md5_byte_t digest[16];
    myMethod(&state, digest, string);
    
    //convert the hashed string to characters for formatting purposes
    for(i = 0; i < 16; i++)
    	sprintf(hashBuffer+(i*2), "%02x", digest[i]);   
    printf("Hash result: \"%s\"\n\n", hashBuffer); //check
    
    //Send "username:hash" to server
    sprintf(sndBuffer, "%s:%s", username, hashBuffer);   //setup send buffer
    printf("Sending \"%s\" to server.\n\n", sndBuffer);    //check 
    totalBytesSent = 0;
    while (totalBytesSent < usernameLen+1+32) //send all the bytes
    {
       	//increment the pointer/decrement the size is message is fragmented
        if ((bytesSent = send(sock, sndBuffer+totalBytesSent,
	MSGBUFSIZE-totalBytesSent, 0)) <= 0)
            DieWithError("send() failed or connection closed prematurely");
        totalBytesSent += bytesSent;   
    }
    
    //Receive authentication status message
    totalBytesRcvd = 0;
    while (totalBytesRcvd < 14) //smallest response size is 14
    {
       	//increment the pointer/decrement the size is message is fragmented
        if ((bytesRcvd = recv(sock, rcvBuffer+totalBytesRcvd, 
        MSGBUFSIZE - 1 - totalBytesRcvd, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely");
        totalBytesRcvd += bytesRcvd; 
    }
    rcvBuffer[totalBytesRcvd]='\0';
    printf("Received Message: \"%s\"\n\n",rcvBuffer);
    
    puts("Closing connection.");
    close(sock);
    exit(0);
}
