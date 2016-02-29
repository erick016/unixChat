/************************************************************/
/*Server For Chat Program                                   */
/*James Erickson                                            */
/*compilation gcc -g chatserverC.c -o clientserverC -pthread*/
/************************************************************/
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

//extended message structure allows easier info passing between client & server
struct  __attribute__ ((__packed__)) message
{
  
  msg_type myType;
  char payload[MSGSIZE];
  int targetSockFD;

  mode_type parentMode; 

  bool nameDuplicate; 

  char targetUser[MSGSIZE]; 
  bool accept;
  char senderUser[MSGSIZE];
};

//user structure to store data
struct user {
  char userName[MSGSIZE];
  int sockfd;
  //uint32_t myIP;
  pthread_t tid;
  int myIndex;
};

struct user empty = {"",-2,NULL,-1}; //values to initialize to later

struct user users[MAXUSERS];

bool emptySlot[MAXUSERS]; //list of slots corresponding with list of users


/*********************************************************/
/*ARGUMENTS:                                             */
/* arg: void pointer to user struct                      */
/*FUNCTION PURPOSE: thread for one user                  */
/*********************************************************/
static void *do_child(void *arg)
{
  struct message msg;

  struct user *userPtr = arg;

  int myIndex =  userPtr -> myIndex ;

  int sockfd = users[myIndex].sockfd;

  printf("%d %d\n",sockfd, myIndex);

  int i = 0;
  int j = 0;

  int k = 0;

  int nbytes;


  for(; ;)
    {
      i = 0;
      nbytes = recv(sockfd, &msg, sizeof(msg) ,0);

      if (nbytes == 0) //if the connection is closed 
	{
	  
	  pthread_exit((void *)0);
	  
	}
     
     switch(msg.myType)
       {
       case MYNAME:
	 
	 for(i = 0; i < MAXUSERS; i++)
	   {
	     if (strncmp(users[i].userName,msg.payload,MSGSIZE) == 0)
	       {
		 msg.nameDuplicate = true;
	       }
	   }

	 i = 0;

	 

	 if(!msg.nameDuplicate)
	   {

	     strcpy(users[myIndex].userName, msg.payload);
	     printf("%d %d %s \n",myIndex ,nbytes , users[myIndex].userName);
	   
	   }

	 send(sockfd, &msg, sizeof(msg),0);

	 break;
       
       case LISTNAMES:
	 
	 for(i = 0; i < MAXUSERS; i++) 
	   {
	     
		 strcpy(msg.payload, users[i].userName);
		 send(sockfd, &msg, sizeof(msg),0);
	       
	   }

	 break;
	 
       case REQUESTCHAT:
	
	 for (i = 0; ( (i < MAXUSERS) && (strncmp(users[i].userName,msg.targetUser,MSGSIZE) != 0) ); i++){} //linear search for matching uname
	   
	   if( strncmp(users[i].userName,msg.targetUser,MSGSIZE) != 0) // if no match was found
	     {
	       strcpy(msg.payload, "This is not an existing username: ");
	       strcat(msg.payload, msg.targetUser);
	       strcat(msg.payload, "\n");

	       printf("%d \n",i);
 
	       msg.targetSockFD = -2; //Send failure to find sockfd
	       send(sockfd, &msg, sizeof(msg),0);
	     }
	   else
	     { 
	       send(users[i].sockfd, &msg, sizeof(msg),0); // Send request to third party
	       msg.targetSockFD = users[i].sockfd; //Indicates success to client
	     }

	 break;

       case ACCEPT:

	 printf(">%d< \n",msg.accept);
	       
	 for (i = 0; ( (i < MAXUSERS) && (strncmp(users[i].userName,msg.targetUser,MSGSIZE) != 0) ); i++){}
	 for (j = 0; ( (j < MAXUSERS) && (strncmp(users[j].userName,msg.senderUser,MSGSIZE) != 0) ); j++){}//find sender
      
	       if (msg.accept)//if accepted
		 {
		   strcpy(msg.payload, "Request accepted. Start chatting. \n");
		   msg.myType = ACCEPT;
		     
		   
		   
		   msg.targetSockFD = users[i].sockfd; //different targets for the two clients 

		   send(sockfd, &msg, sizeof(msg),0);

		   msg.targetSockFD = sockfd; 

		   send(users[i].sockfd, &msg, sizeof(msg),0);
		 }
	       else 
		 {
		   strcpy(msg.payload, "Request to chat denied. \n ");
		   msg.myType = ACCEPT;
		   msg.targetSockFD = -2; //Send failure to find sockfd
		   send(sockfd, &msg, sizeof(msg),0);
		   send(users[i].sockfd, &msg, sizeof(msg),0);
		 }

	 break;

       case MESSAGE:
	 
	 send(msg.targetSockFD, &msg, sizeof(msg), 0);
	 
	 break;

       case EXIT:

	 for (j = 0; ( (j < MAXUSERS) && (strncmp(users[j].userName,msg.senderUser,MSGSIZE) != 0) ); j++){}

	 emptySlot[j] = true; // free up space in the user queue
	 users[j] = empty;


	 break;
       }
    } 

}


int main(int argc, char **argv)
{
  int listenfd;

  struct sockaddr_in servaddr, clientaddr;  
  
  struct message msg;

  socklen_t clientaddrSize = sizeof(clientaddr);

  int i = 0;
  
  int j;
  for (j = 0; j < MAXUSERS; j++) 
    {  
      users[j] = empty; //initiaize the user array
      emptySlot[j] = true; //everything is empty
    }
  
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
    
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
  servaddr.sin_port = htons(PORTNUM);
    
  bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
    
  listen(listenfd, LISTENQ);
    
  for(; ;)
    {
      
      i = 0;

      while (!emptySlot[i] && i < MAXUSERS)
	{
	  i++;
	}
      

      if (i < MAXUSERS)
	{
	  
	  users[i].sockfd = accept(listenfd, (SA *) &clientaddr, &clientaddrSize);
          
	  users[i].myIndex = i;

	  printf("%d\n",users[i].sockfd);
	  
	  emptySlot[i] = false;

	  pthread_create(&users[i].tid, NULL, do_child , &users[i]);
	
	}
      }
}

