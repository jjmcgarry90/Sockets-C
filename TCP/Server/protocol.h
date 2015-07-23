#define HASHBUFSIZE 120   //size of hash string buffer
#define MSGBUFSIZE 255   //max size of any message sent

#define DELIM (const char*)(" ") //delimiter for messages
#define AUTH_REQUEST (char*)("requesting_authentication") //initial request
#define REQUEST_HASH (char*)("requesting_hash") //server wants a hash
#define HASH_TO_SERVER (char*)("username_and_hash:") //server gets a hash
#define AUTH_SUCCESS (char*)("successful_login")
#define AUTH_BAD_USR (char*)("user_not_found")  
#define AUTH_BAD_PASS (char*)("incorrect_password")

