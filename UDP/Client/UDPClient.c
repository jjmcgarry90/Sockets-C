/*
This code is a highly modified version of code written in
"TCP/IP Sockets in C : Practical guide for Programmers"
written by Michael J. Donahoo and Kenneth L. Calvert
*/

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and alarm() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */
#include "protocol.h"
#include "md5.h"

#define TIMEOUT_SECS    2       /* Seconds between retransmits */
#define MAXTRIES        5       /* Tries before giving up */

void DieWithError(char *errorMessage);   /* Error handling function */
void CatchAlarm(int ignored);            /* Handler for SIGALRM */
void receiveWithTimer();
void sigHandInit();

int tries=0;   /* Count of times sent - GLOBAL for signal-handler access */
int sock;                        /* Socket descriptor */
struct sockaddr_in echoServAddr; /* Server address */
struct sockaddr_in fromAddr;     /* Source address of incoming message  */
unsigned short echoServPort;     /* Server port */
unsigned int fromSize;           /* In-out of address size for recvfrom() */
struct sigaction myAction;       /* For setting signal handler */
char *servIP;                    /* IP address of server */
char echoString[MSGBUFSIZE+1];      /* String to send to server */
char echoBuffer[MSGBUFSIZE+1];      /* Received string from server */
int echoStringLen;               /* Length of string to send */
int respStringLen;               /* Size of received string from server */

    
int main(int argc, char *argv[])
{
    int running;			 /* program will end when this is set to 0 */
   
    char* username;			 /* username to authenticate */
    char* password;			 /* password to authenticate */
    int userLen, passLen;		 /* length of username and password strings */
    int i;				 /* general purpose loop indexer */
    
    if ((argc!=5))    /* Test for correct number of arguments */
    {
        fprintf(stderr,"Usage: %s <server IP> <server port> <username> <password>\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];          		 /* First arg:  server IP address (dotted quad) */
    echoServPort = atoi(argv[2]);	 /* Second arg: server port */
    username = argv[3];			 /* Third arg: username */
    userLen = strlen(username);
    password = argv[4];			 /* Fourth arg: password */
    passLen = strlen(password);
    
    

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");
	
    /* Set signal handler for alarm signal */
    sigHandInit();
    
    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    echoServAddr.sin_port = htons(echoServPort);       /* Server port */
    
    /* put the first request in the send buffer */
    strcpy(echoString, AUTH_REQUEST);
    
    puts("\n"); //print some space
    
    /* run until we receive authentication status message */
    running=1;
    while(running) {	
    	/* Send message to server */
    	if (sendto(sock, echoString, strlen(echoString), 0, (struct sockaddr *)
        &echoServAddr, sizeof(echoServAddr)) != strlen(echoString))
        	DieWithError("sendto() sent a different number of bytes than expected");
        	
        printf("Sent message: \"%s\"\n\n" , echoString);
        
        /* Try to receive message from server, timeout if no response */
    	receiveWithTimer();
    	echoBuffer[respStringLen] = '\0';	     /* null-terminate the received data */
    	printf("Received message: \"%s\"\n\n", echoBuffer);    /* Print the received data */
    	
 	/* parse the response for the message */
    	char *message = strtok(echoBuffer, DELIM);
    	
    	//if server sent a request for hash
    	if(strcmp(message, REQUEST_HASH)==0) {
    	    char hashBuffer[HASHBUFSIZE];  //to hold the hash
    	    
    	    /* get the random string parameter from the response message */
    	    char* randomString = strtok(NULL,DELIM); 
    	    
    	    //concatenate the username, password, and 64 char string
	    sprintf(hashBuffer, "%s", username);
	    sprintf(hashBuffer+userLen, "%s", password);
	    sprintf(hashBuffer+userLen+passLen, "%s", randomString);
	    printf("String to be hashed: %s\n\n", hashBuffer);
	
	    //md5Hash the concatenated string
	    md5_byte_t* string =  (md5_byte_t*) hashBuffer;
	    md5_state_t state;
	    md5_byte_t digest[16];
	    myMethod(&state, digest, string);
	    
	    //convert the hashed string to characters for formatting purposes
	    for(i = 0; i < 16; i++)
	    	sprintf(hashBuffer+(i*2), "%02x", digest[i]);   
	    printf("Hash result: %s\n\n", hashBuffer); //check
	    
	    //setup send buffer to send hash
	    sprintf(echoString, "%s %s %s %s", 
	    		HASH_TO_SERVER, username, randomString, hashBuffer); 
	    
    	}
    	
    	/* Print the authentication status and end the program */
    	else if(strcmp(message, AUTH_SUCCESS)==0) {
    	    puts("Authentication Verified. Welcome.");
    	    running=0;
    	}
    	
    	else if(strcmp(message, AUTH_BAD_USR)==0) {
    	    puts("Authentication failed: Username does not exist.");
    	    running=0;
    	}
    	
    	else if(strcmp(message, AUTH_BAD_PASS)==0) {
    	    puts("Authentication failed: incorrect password.");
    	    running=0;
    	}
    	
    }
    close(sock);
    exit(0);
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    tries += 1;
}

void receiveWithTimer() {
    fromSize = sizeof(fromAddr);
    alarm(TIMEOUT_SECS);        /* Set the timeout */
    while ((respStringLen = recvfrom(sock, echoBuffer, MSGBUFSIZE, 0,
           (struct sockaddr *) &fromAddr, &fromSize)) < 0)
        if (errno == EINTR)     /* Alarm went off  */
        {
            if (tries < MAXTRIES)      /* incremented by signal handler */
            {
                printf("timed out, %d more tries...\n", MAXTRIES-tries);
                /* resend the message */
                if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)
                            &echoServAddr, sizeof(echoServAddr)) != echoStringLen)
                    DieWithError("sendto() failed");
                alarm(TIMEOUT_SECS);
            } 
            else
                DieWithError("No Response");
        } 
        else
            DieWithError("recvfrom() failed");

    /* recvfrom() got something --  cancel the timeout */
    alarm(0);
}

void sigHandInit() {
    myAction.sa_handler = CatchAlarm;
    if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
        DieWithError("sigfillset() failed");
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0)
        DieWithError("sigaction() failed for SIGALRM");
}
