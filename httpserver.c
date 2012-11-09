#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

void makeConnection();
void logSet();

int server_socket = 0;
int accept_socket = 0;

void main(){

  logSet();
  makeConnection();

}



void makeConnection(){
  
   char recv_Buff[3000];
   struct sockaddr_in server_adder;
   struct sockaddr_in accept_adder;

   printf("Hello, make socket\n");

   memset(recv_Buff, 0, sizeof(recv_Buff));

   server_socket = socket(AF_INET, SOCK_STREAM, 0);
   if(server_socket == -1){
      fprintf(stderr, "server_socket Error!");
      exit(1);
   }

   memset(&server_adder, 0, sizeof(server_adder));
   server_adder.sin_family		= AF_INET;
   server_adder.sin_addr.s_addr		= htonl(INADDR_ANY);
   server_adder.sin_port		= htons(8000);

   if(-1 == bind(server_socket, (struct sockaddr *) &server_adder, sizeof(server_adder)) ){
      fprintf(stderr, "Error Bind!");
      exit(1);
   }
 
   
   if(-1 == listen(server_socket, 10) ){
      fprintf(stderr, "Error listen!");
      exit(1);
   }

   int accept_adder_size = sizeof(accept_adder);

   while(1){
   
      accept_socket = accept(server_socket, (struct sockaddr *) &accept_adder, &accept_adder_size);
      if(accept_socket == -1){
          fprintf(stderr, "accept_socket Error!");
	  exit(1);
      }

      if(-1 == recv(accept_socket, recv_Buff, sizeof(recv_Buff), 0) ){
          fprintf(stderr, "recv Error!");
      }
 
      printf("receive : \n%s\n",recv_Buff);
   
}


void logSet(){
   
   freopen("/home/log", "a", stderr);

}
