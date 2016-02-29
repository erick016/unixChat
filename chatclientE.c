/************************************************************/
/*Client For Chat Program                                   */
/*James Erickson                                            */
/*compilation gcc -g chatclientE.c -o clientclientE -pthread*/
/************************************************************/
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#define LISTENQ 1024
#define MAXLINE 4096
#define BUFFSIZE 8192
#define SA      struct sockaddr
#define MSGSIZE 512
#define MAXUSERS 10
#define PORTNUM 1024

typedef enum {MYNAME,LISTNAMES,REQUESTCHAT,ACCEPT,MESSAGE,EXIT} msg_type;
typedef enum {PROMPT,LIST,WHOM,ACCEPTREQUEST,CHATTING,CTRLC,LEAVECHAT} mode_type;

//global variables used to pass important information between threads

mode_type mode;
char targetUser[MSGSIZE];
int partnerSockFD;

//extended message structure allows easier info passing between client & server
struct  __attribute__ ((__packed__)) message {
  
  msg_type myType;
  char payload[MSGSIZE];
  int targetSockFD;

  mode_type parentMode;

  bool nameDuplicate; 

  char targetUser[MSGSIZE]; 
  bool accept;
  char senderUser[MSGSIZE];
};

/*********************************************************/
/*ARGUMENTS:                                             */
/* signum: integer going along with SIGINT as in exit(0) */
/*FUNCTION PURPOSE: handles the SIGINT                   */
/*********************************************************/
void signal_callback_handler(int signum)
{

   if (mode == PROMPT)
     {
       printf ("\nExited chat client. Press Enter.  \n");
       mode = CTRLC;
     }
   if (mode == CHATTING)
     {
       printf ("\nEnded the current conversation. Press Enter.  \n");
       mode = LEAVECHAT;
     }

}

/*********************************************************/
/*ARGUMENTS:                                             */
/* msg: string to go along with ERROR                    */
/*FUNCTION PURPOSE: handles errors in intial conn setup  */
/*********************************************************/
void error(const char *msg)
{
  perror(msg);
  exit(0);
}

/*********************************************************/
/*ARGUMENTS:                                             */
/* arg: void pointer to sockFD of server                 */
/*FUNCTION PURPOSE: runs as a thread to display messages */
/*********************************************************/
static void *do_recv(void *arg)
{

  struct message msg;

  int *sockfd = arg;

  char c[MSGSIZE];

  int i;

  for( ; ; )
    {
      recv(sockfd, &msg, sizeof(msg), 0);
      
      switch(msg.myType)
	{
	
	case LISTNAMES:

	  printf("\nUsers: \n%s\n",msg.payload);

	  for (i = 1; i < MAXUSERS; i++)
	    {
	      recv(sockfd, &msg, sizeof(msg), 0);
	      if (strncmp(msg.payload, "", MSGSIZE) != 0)
		printf("%s \n",msg.payload);
	    }

	  if (mode != WHOM)
	    {
	      mode = PROMPT;
	      printf("\r\n1.List Users \r\n2.Chat \r\n3.Exit \r\n \r\n \r\n");
	    }
	  break;

	
	case REQUESTCHAT:
	  
	  if (mode == PROMPT)
	    {

	      printf("%s \n",msg.payload);

	      if (msg.targetSockFD == -2) //if we couldn't find the username
		{
		  mode = PROMPT;
		  printf("\r\n1.List Users \r\n2.Chat \r\n3.Exit \r\n \r\n \r\n");
		}

	      else 
		{
		  strcpy(targetUser, msg.senderUser); //sender of old message is global target

		  mode = ACCEPTREQUEST;
		}
	    }


	  else
	    {
	      msg.myType = ACCEPT;
	      strcpy(msg.payload, "User cannot chat right now. \n");

	      strcpy(msg.targetUser, msg.senderUser);
	      send(sockfd, &msg, sizeof(msg), 0);
	    }
	 
	  break;
	
	case ACCEPT:
	  
	  printf("%s \n",msg.payload);
	  
	  if (msg.targetSockFD == -2) //if our request to chat was denied
	    {
	      if (mode != CHATTING)
		{
		  mode = PROMPT;
		  printf("\r\n1.List Users \r\n2.Chat \r\n3.Exit \r\n \r\n \r\n");
		}
	    }

	  else
	    {
	      mode = CHATTING;
	      partnerSockFD = msg.targetSockFD; //make visible to loop
	    }

	  break;
	
	  case MESSAGE:

	    printf("%s \n", msg.payload);

	    if (msg.parentMode == LEAVECHAT)
	      mode = PROMPT;

	    break;

	}
    
    }

  pthread_exit((void *)0);
}

/*********************************************************/
/*ARGUMENTS:                                             */
/* sockfd: sockfd of server                              */
/* myName: string of client's name                       */
/*FUNCTION PURPOSE: send user name to server             */
/*********************************************************/
static bool send_userName(char *myName, int sockfd)
{
  struct message msg;
  msg.myType = MYNAME;
  msg.nameDuplicate = false;
  strcpy(msg.payload, myName);

  send(sockfd, &msg, sizeof(msg),0);
  
  recv(sockfd, &msg, sizeof(msg),0);

  return msg.nameDuplicate;

}

/*********************************************************/
/*ARGUMENTS:                                             */
/* sockfd: sockfd of server                              */
/*FUNCTION PURPOSE: list clients                         */
/*********************************************************/
static void getUsers(int sockfd)
{
  struct message msg;
  msg.myType = LISTNAMES; 


  send(sockfd, &msg, sizeof(msg), 0);
  
  int i;

  
}

/*********************************************************/
/*ARGUMENTS:                                             */
/* myName: client's name                                 */
/* requested: name of client we want to chat with        */
/* sockfd: sockfd of server                              */
/*FUNCTION PURPOSE: handles errors in intial conn setup  */
/*********************************************************/
static void requestChat(char *myName ,char *requested,int sockfd)
{

  
  struct message msg;
  msg.myType = REQUESTCHAT;
  msg.accept = false;
  strcpy(msg.senderUser,myName);
  strcpy(msg.targetUser,requested);
  

  strcpy(msg.payload, myName);
  strcat(msg.payload, " would like to chat. Enter Y to accept, N to reject. \n");

  send(sockfd, &msg, sizeof(msg),0);
}


int main(int argc, char **argv)
{
  signal(SIGINT, signal_callback_handler);

  int sockfd, i;

  i = 0;

  mode = PROMPT; //Set it up so that the for loop will start in Prompt mode.

  char *myName = malloc(MSGSIZE);
  char *requested = malloc(MSGSIZE);

  struct message msg;

  char c;
  
  struct sockaddr_in servaddr;
    
  int status;
    
  pthread_t rcvThread;

  char *success; //infinite loop stuff
  char *x = malloc(2); 
    
  if(argc != 2)
    {
      printf("Error: expected IP address argument");
      exit(1);
    }
  if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        
      error("Socket error");
    }


    
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORTNUM);
    
  if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <=0)
    {
      printf("inet_pton error for %s \n", argv[1]);
      exit(3);
    }
    
  if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
    {
      error("Connect error");
    }

  printf("Type in a username: \r\n");

  for( ; ; )
    {

      while ( (myName[i++] = getchar()) != EOF && myName[i - 1] != '\n' && i < MSGSIZE ){}
	
      myName[i - 1] = '\0';
      i = 0;

      printf(">%s<\n",myName);
      if(!send_userName(myName,sockfd)) // if no duplicate name.
	break;
      else
	printf("That user name already exists. Type in another one. \r\n");
	
      

    }

//IF WE MADE IT INTO THE USER QUEUE
  
  
  pthread_create(&rcvThread, NULL, do_recv, (void *) sockfd);

  struct message loopMsg;
  char *chatMessage = malloc(MSGSIZE);

  for( ; ; )
    {

      loopMsg.accept = false;

      loopMsg.targetSockFD = -2;

      if (mode == CTRLC)
	{
	  loopMsg.myType = EXIT;
	  strcpy(loopMsg.senderUser, myName);
	  send(sockfd, &loopMsg, sizeof(loopMsg),0);

	  exit(0);
	}

      if (mode == LEAVECHAT)
	{
	  loopMsg.targetSockFD = partnerSockFD; //get from global
	      loopMsg.myType = MESSAGE;
	      

	      loopMsg.parentMode = LEAVECHAT;
	      
	      strcpy(loopMsg.payload, "\nPartner has ended chat session. Press Enter. \n");


	      send(sockfd, &loopMsg, sizeof(loopMsg), 0);

	      mode = PROMPT;
	}

      if (mode == PROMPT)
	{
	  printf("\r\n1.List Users \r\n2.Chat \r\n3.Exit \r\n \r\n \r\n");
	}
	 

      success = fgets(x, 512, stdin); 

	  if ((strncmp (x,"1",1) == 0) && mode == PROMPT)
	    {
	      mode = LIST;
	      getUsers(sockfd);
	    }
      
	  if (strncmp (x,"2",1) == 0 && mode == PROMPT)
	    {
	  
	      mode = WHOM;
	      getUsers(sockfd);

	      
	      i = 0;
	      
	      printf("To whom do you wish to chat? \r\n");

	      while ( (requested[i++] = getchar()) != EOF && requested[i - 1] != '\n' && i < MSGSIZE && ( strncmp(requested,myName,MSGSIZE) != 0) ){}
	      requested[i - 1] = '\0';
	      i = 0;
	      

	      printf(">%s<\n",requested);
	  
	      requestChat(myName,requested,sockfd);
	      
	    }

	  if (strncmp (x,"3",1) == 0 && mode == PROMPT)
	    {
	      loopMsg.myType = EXIT;
	      strcpy(loopMsg.senderUser, myName);
	      send(sockfd, &loopMsg, sizeof(loopMsg),0);
	      
	      exit(0);
	    }
      

      if (mode == ACCEPTREQUEST)
	{

	  if ((strncmp(x,"y",1) == 0) || (strncmp(x,"Y",1) == 0) || (strncmp(x,"n",1) == 0) || (strncmp(x,"N",1) == 0) )
	    {
	      if ((strncmp(x,"y",1) == 0) || (strncmp(x,"Y",1) == 0))
		{  
		  loopMsg.accept = true;
		  mode = CHATTING;
		}

	      if ((strncmp(x,"n",1) == 0) || (strncmp(x,"N",1) == 0))
		{
		  //mode = PROMPT;
		}
	      loopMsg.myType = ACCEPT;

	      strcpy(loopMsg.targetUser, targetUser);


	      strcpy(loopMsg.senderUser, myName);
	 
	      send(sockfd, &loopMsg, sizeof(loopMsg), 0);

	      x[0] = '\0'; //empty x.
	
	      loopMsg.accept = false;
	      continue; //don't go into chat mode directly after this

	    }	  
	}
    
      if (mode == CHATTING)
	{
	  if (partnerSockFD != -2) // if we have a partner
	    {
	      loopMsg.targetSockFD = partnerSockFD; //get from global
	      loopMsg.myType = MESSAGE;
	      
	      strcat(chatMessage,myName);
	      strcat(chatMessage,": ");
	      strcat(chatMessage,x);
	      
	      strcpy(loopMsg.payload, chatMessage);

	      strcpy(chatMessage, ""); //blank out chatmessage for next time

	      send(sockfd, &loopMsg, sizeof(loopMsg), 0);
	    }
	}

      
    }
}



