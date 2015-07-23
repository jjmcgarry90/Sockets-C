#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "protocol.h"
#include "md5.h"

#define RANDBUFSIZE 64

void DieWithError(char *errorMessage);  /* Error handling function */

void HandleTCPClient(int clntSocket, char* username, char* password)
{
    char inMsg[MSGBUFSIZE+1];             /* Buffer for received string */
    char outMsg[MSGBUFSIZE+1];	          /* Buffer for sent string */
    int bytesRcvd, bytesSent;             /* Size of received message */
    int totalBytesRcvd, totalBytesSent;   /* Total bytes received, sent */
    char randChars[RANDBUFSIZE+1];		  /* Hold random string */
    unsigned int iseed;			  /* Random seed value */
    int expectedMsgLen;			  /* expected response length */
    char clientHash[HASHBUFSIZE];	  /* holds client's hash string */
    char serverHash[HASHBUFSIZE];	  /* holds server's hash string */
    
    
    /* usernames and passwords that the system supports */
    char* credentials[6] = {username, password, "Bob", "123", "Sally", "111"};
    
    /*expected length of first message, to ensure we get all the bytes */
    expectedMsgLen = strlen(AUTH_REQUEST); 
    //Receive authentication request
    totalBytesRcvd = 0;
    while (totalBytesRcvd < expectedMsgLen) { //ensure all bytes are received
	    if ((bytesRcvd = recv(clntSocket, inMsg+totalBytesRcvd,
	    				      MSGBUFSIZE-totalBytesRcvd, 0)) < 0)
    		DieWithError("recv() failed");
	    totalBytesRcvd+= bytesRcvd;
    }

    inMsg[totalBytesRcvd] = '\0';	
    printf("Received Message: \"%s\"\n\n", inMsg);   //check
    
    //generate a random 64-character string based on time
    iseed = (unsigned int)time(NULL);
    srand (iseed);
    int i;
    for(i=0; i<RANDBUFSIZE; i++) {
   	 randChars[i] = (char)((rand() % 26)+ 97); // = a random, lower-case letter
    }
    randChars[RANDBUFSIZE] = '\0';	//terminate the string
    strcpy(outMsg, randChars); //copy the random string to the send buffer
    printf("Sending random string: \"%s\"\n\n", outMsg);  //check
    
    
    //send the random string
    totalBytesSent = 0;
    while (totalBytesSent < strlen(outMsg)) //ensure all bytes are sent
    {
        if ((bytesSent = send(clntSocket, outMsg + totalBytesSent,
	    			    strlen(outMsg) - totalBytesSent, 0)) <= 0)
            DieWithError("send() failed or connection closed prematurely");
        totalBytesSent += bytesSent;   /* Keep tally of total bytes */
    }
    
    //receive the "username:hash" from the Client
    totalBytesRcvd = 0;
    expectedMsgLen = strlen(username)+1+32; //username + ':' + 32 byte hash string
    while (totalBytesRcvd < expectedMsgLen) { //ensure all bytes are received
	    if ((bytesRcvd = recv(clntSocket, inMsg+totalBytesRcvd,
	    				      MSGBUFSIZE-totalBytesRcvd, 0)) < 0)
    		DieWithError("recv() failed");
	    totalBytesRcvd+= bytesRcvd;
    }

    inMsg[totalBytesRcvd] = '\0';	//terminate the string
    printf("Received Message: \"%s\"\n\n", inMsg); //print received data
    
    //split the string into username and hash
    char* delim = ":";  //delimiter for split
    username  = strtok(inMsg,delim);
    strcpy(clientHash, strtok(NULL, delim));

    printf("Split message into \"%s\" and \"%s\"\n\n", username, clientHash);
    
    //see if username is on file, checking every other string in credentials
    fputs("Checking Username...",stdout);
    int found = 0;
    for (i=0; i<sizeof(credentials)/4; i+=2) //i < number of strings in credentials
    	if(strcmp(username, credentials[i])==0) { //if found
    	    found = 1;
    	    password = credentials[i+1];  //pull the password
    	} 
    
    //if username is valid
    if (found)	{
    	    printf("found %s\n\n",username);
  
	    //concatenate username, password, and 64 character string
	    sprintf(serverHash, "%s", username);
	    sprintf(serverHash+strlen(username), "%s", password);
	    sprintf(serverHash+strlen(username)+strlen(password), "%s", randChars);
	    printf("String to be hashed: \"%s\"\n\n", serverHash);
	    
	    //hash the concatenation
	    md5_byte_t* string =  (md5_byte_t*) serverHash;
	    md5_state_t state;
	    md5_byte_t digest[16];
	    myMethod(&state, digest, string);
	    
	     for(i = 0; i < 16; i++)
	    	sprintf(serverHash+(i*2), "%02x", digest[i]);   
	    printf("Hash result: \"%s\"\n\n", serverHash);
	    
	    //compare client and server hash results
	    if (strcmp(serverHash, clientHash)==0) { //if they match
	    	puts("Successful login, notifying client.\n");
	    	/* prepare send buffer */ 
	    	strcpy(outMsg,"Successful Login"); 
	    }
            else { //username was found, so must be a bad password
            	puts("Bad password, notifying client.\n");
            	/* prepare send buffer */
            	strcpy(outMsg,"Authentication failed: incorrect password");
            }
    }
    
    //username is not valid
    else {
    	puts("Bad username, notifying client.\n");
    	/* prepare send buffer */
    	sprintf(outMsg, "The username %s does not exist", username);
    }
    
    //send authentication status
    totalBytesSent = 0;
    while (totalBytesSent < strlen(outMsg)) //ensure all bytes are sent
    {
        if ((bytesSent = send(clntSocket, outMsg + totalBytesSent,
	    			    strlen(outMsg) - totalBytesSent, 0)) <= 0)
            DieWithError("send() failed or connection closed prematurely");
        totalBytesSent += bytesSent;   /* Keep tally of total bytes */
    }
    
    puts("Finished. Closing connection.");
  	
    close(clntSocket);    /* Close client socket */
}	
