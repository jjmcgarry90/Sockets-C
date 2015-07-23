Jessie McGarry
jmcgarry3@gatech.edu

FILES: 
TCPClient.c 		- The TCP Client Program
TCPServer.c		- The TCP Server Program
HandleTCPClient.c	- Handler used by TCP Server Program, handles one connection
UDPClient.c		- The UDP Client Program
UDPServer.c		- The UDP Server Program
DieWithError.c		- Exits when there is an error, used by both clients and both servers
md5.c			- Provides MD5 Hash functionality
md5.h			- header for MD5
protocol.h		- my protocol, included by both clients and both servers
Makefile		- can create and delete the executable from source code
README.txt 		- this

INSTRUCTIONS:
1. in the 'AssignmentOne' directory, type 'make'
2. the executables for all of the programs will be created in the same directory
3. type 'make clean' to delete the executables
4. Run a server execuctable in the form "./server-*** <Port to run on> <username to support> <Password to support>
4. Run its correponding client in the form "./letmein-*** <Server IP> <Server Port> <Username> <Password>

APPLICATION PROTOCOL:
The request/response format is <message><sp><parameters>
Additional parameters are specified using additional spaces

The general sequence for both programs is as follows:
1. Client sends the text "requesting_authentication" to the server
2. Server responds with "requesting_hash <random, lower case 64 character string>"
3. Client responds with "username_and_hash: <username> <random string> <hash>"
4. Server responds with "successful_login", "user_not_found", or "incorrect_password"

The TCP version follows a sequential send-receive pattern once the initial authentication request is made, using a persistent connection. Because of this linear layout, TCP version knows what to expect after it has sent a message (except at the very end, where the server's authentication status response coud vary), and it has no need to follow the exact message format. I did leave it open-ended, however, so the server could be made to carry out other requests besides authentication through TCP, simply by expanding the protocol.

The UDP version, however, strictly adheres to the protocol outline above. The programs run within loops, processing each request/response differently according to what the <message> field dictates. Although the server runs forever, the client is able to end its process after receiving one of the authentication status messages from the server. The UDP follows an open-ended design as well, so that the server can be made to handle other types of requests.

For reference, this is my protocol.h:

#define HASHBUFSIZE 120   //size of hash string buffer
#define MSGBUFSIZE 255   //max size of any message sent

#define DELIM (const char*)(" ") //delimiter for messages
#define AUTH_REQUEST (char*)("requesting_authentication") //initial request
#define REQUEST_HASH (char*)("requesting_hash:") //server wants a hash
#define HASH_TO_SERVER (char*)("username_and_hash:") //server gets a hash
#define AUTH_SUCCESS (char*)("successful_login")
#define AUTH_BAD_USR (char*)("user_not_found")  
#define AUTH_BAD_PASS (char*)("incorrect_password")


LIMITATIONS
1. The total size of username and password together cannot be greater than 56 characters.
   The hash string buffer can't hold more than that in addition to the hash string.
2. No message sent between the clients and servers can be greater than 255 characters.
   If protocol is followed, no message will be.
3. The credentials specified when starting the server program are always supported. However, additional accounts currently have to be hard-coded. Thus, if a client wants to authenticate an account that the server starter didn't specify when starting the server, that account must exist in the 'credentials' field in the server program.

