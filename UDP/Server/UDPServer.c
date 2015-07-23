/*
This code is a highly modified version of code written in
"TCP/IP Sockets in C : Practical guide for Programmers"
written by Michael J. Donahoo and Kenneth L. Calvert
*/

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <time.h>       /* for time() */
#include "md5.h"        /* for hashing */
#include "protocol.h"   /* for communicating */

void DieWithError(char *errorMessage);  /* External error handling function */

int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    char echoBuffer[MSGBUFSIZE+1];   /* Buffer for received string */
    unsigned short echoServPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
    char randString[64+1];	     /* Random, 64-character string */
    unsigned int iseed;		     /* Random generator seed value */
    int i;			     /* Loop indexer */
    char clientHash[HASHBUFSIZE];    /* Hash received from client */
    char serverHash[HASHBUFSIZE];    /* Hash produced by Server */
    char* username;		     /* Username to support */
    char* password;		     /* Password to support */
    

    if (argc != 4)         /* Test for correct number of parameters */
    {
        fprintf(stderr,"Usage:  %s <UDP SERVER PORT> <USERNAME> <PASSWORD>\n", argv[0]);
        exit(1);
    }
    
    echoServPort = atoi(argv[1]);  /* First arg:  local port */
    username = argv[2];		   /* Second arg: username the server should support */
    password = argv[3];		   /* Third arg: password the server should support */
    
    /* These are the users and passwords that the system supports */
    char *credentials[] = {username, password, "user2", "password2", "user3", "password3"}; 
    
    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");
        
    puts("\n"); //print some space
    
    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(echoClntAddr);
	
	puts("Waiting for request...\n");
        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, echoBuffer, MSGBUFSIZE, 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");
	echoBuffer[recvMsgSize] = '\0';  //terminate the received date 
        
        /* print the received data and who it came from */
	printf("Received: \"%s\" from %s\n\n", echoBuffer, inet_ntoa(echoClntAddr.sin_addr));
	
	//parse the message (spaces separate message and parameters)
	char* message = strtok(echoBuffer,DELIM);
	
	/* if client request authentication, send them a random,
	 lower-case 64-character sting */
	if (strcmp(message ,AUTH_REQUEST)==0) {
	    
	    //generate a random 64-character string based on time
	    iseed = (unsigned int)time(NULL);
	    srand (iseed);
	  
	    for(i=0; i<64; i++) 
	  	 randString[i] = (char)((rand() % 26)+ 97); //assigns a lower-case letter
	    randString[64] = '\0'; //terminate the string
	    printf("Sending random string: %s\n\n", randString);  //print the string
	    sprintf(echoBuffer, "%s %s", REQUEST_HASH, randString); //put string in send buffer
 	} 
 	
 	/* Else if we have received a hash from the client */
 	else if (strcmp(message, HASH_TO_SERVER) == 0) {
 	    username = strtok(NULL, DELIM);	//username is the first parameter
 	    strcpy(randString, strtok(NULL, DELIM)); //the random string is the second
 	    strcpy(clientHash, strtok(NULL, DELIM)); //the hash is third
 	    
 	    /* search our credentials file for the username */
 	    int found = 0;
	    for (i=0; i<(sizeof(credentials)/4); i+=2) //i < number of strings in credentials
	    	if(strcmp(username, credentials[i])==0) { //found the username
	    	    found = 1;
	    	    password = credentials[i+1];  //pull the password
	    	} 
	    	
	    if (found)	{
		    //concatenate username, password, and 64 character string
		    sprintf(serverHash, "%s", username);
		    sprintf(serverHash+strlen(username), "%s", password);
		    sprintf(serverHash+strlen(username)+strlen(password), "%s", randString);
		    printf("String to be hashed: %s\n\n", serverHash);
		    
		    //hash the concatenation
		    md5_byte_t* string =  (md5_byte_t*) serverHash;
		    md5_state_t state;
		    md5_byte_t digest[16];
		    myMethod(&state, digest, string);
		    
		    //print the hash result
		     for(i = 0; i < 16; i++)
		    	sprintf(serverHash+(i*2), "%02x", digest[i]);   
		    printf("Hash result: %s\n\n", serverHash);
		    
		    /* compare client and server hash results and
		       send and appropriate response */
		    if (strcmp(serverHash, clientHash)==0) {    //hashes match
		    	puts("Authentication verified. Sending message.\n");
		    	strcpy(echoBuffer, AUTH_SUCCESS); //success message to send buffer
		    }
		    else {	//username was found, so it must be a bad password
		    	puts("Authentication failed: bad password. Sending message.\n");
		    	strcpy(echoBuffer, AUTH_BAD_PASS); //fail message to send buffer
		    }
   	    }
    
    
	    else {     //username wasn't found
	    	puts("Authentication failed: bad username. Sending message.\n");
	    	strcpy(echoBuffer, AUTH_BAD_USR);  //fail message to send buffer
	    }  
 	}
 	    
        /* Send received datagram back to the client */
        if (sendto(sock, echoBuffer, strlen(echoBuffer), 0, 
             (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != strlen(echoBuffer))
            DieWithError("sendto() sent a different number of bytes than expected");
    }
    /* NOT REACHED */
}
