/* tcpserver.c */
/* Programmed by Theo Fleck */
/* October 23, 2019 */    

#include <ctype.h>          /* for toupper */
#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset */
#include <sys/socket.h>     /* for socket, bind, listen, accept */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 1024   
#define BUFFER_SIZE 80

/* SERV_TCP_PORT is the port number on which the server listens for
   incoming requests from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

#define SERV_TCP_PORT 6680

short getShortFromChars(char* inputData, int start, int end){
  //pull 2 chars from provided array and put into a single short
  uint16_t num;
  char *num_data = (char*)&num;
  num_data[0] = inputData[start];
  num_data[1] = inputData[end];
  short retNum = ntohs(num);
  return retNum;
}

int main(void) {
  //define all socket variables
  int sock_server;  /* Socket on which server listens to clients */
  int sock_connection;  /* Socket on which server exchanges data with client */

  struct sockaddr_in server_addr;  /* Internet address structure that
                                      stores server address */
  unsigned int server_addr_len;  /* Length of server address structure */

  struct sockaddr_in client_addr;  /* Internet address structure that
                                      stores client address */
  unsigned int client_addr_len;  /* Length of client address structure */

  /* open a socket */

  if ((sock_server = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("Server: can't open stream socket");
    exit(1);                                                
  }

  /* initialize server address information */

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  /* This allows choice of
                                      any host interface, if more than one
                                      are present */ 
  unsigned short server_port = SERV_TCP_PORT; /* Server will listen on this port */
  server_addr.sin_port = htons(SERV_TCP_PORT);

  /* bind the socket to the local server port */

  if (bind(sock_server, (struct sockaddr *) &server_addr,
           sizeof (server_addr)) < 0) {
    perror("Server: can't bind to local address");
    close(sock_server);
    exit(1);
  }                     

  /* listen for incoming requests from clients */

  if (listen(sock_server, 50) < 0) {    /* 50 is the max number of pending */
    perror("Server: error on listen"); /* requests that will be queued */
    close(sock_server);
    exit(1);
  }
  printf("I am here to listen ... on port %hu\n\n", server_port);

  client_addr_len = sizeof (client_addr);

  /* wait for incoming connection requests in an indefinite loop */

  sock_connection = accept(sock_server, (struct sockaddr *) &client_addr, 
                           &client_addr_len);
  /* The accept function blocks the server until a
                    connection request comes from a client */
  if (sock_connection < 0) {
    perror("Server: accept() error\n"); 
    close(sock_server);
    exit(1);
  }

  //receive filename packet
  //receive and decode header
  char sockMsg[4];
  int bytes_recd = recv(sock_connection, sockMsg, 4, 0);
  if(bytes_recd < 0){
    perror("Filename header receive error");
    exit(1);
  }
  short count = getShortFromChars(sockMsg, 0, 1);
  short sequenceNum = getShortFromChars(sockMsg, 2, 3);
  
  //receive filename data
  char filename[count+1];  //extra space for null terminator
  bytes_recd = recv(sock_connection, filename, sizeof(filename)-1, 0);
  filename[count+1] = '\0';
  if(bytes_recd < 0){
    perror("Filename data receive error");
    exit(1);
  }

  if (bytes_recd > 0){
    printf("Requested filename: %s\n", filename);

    int totalBytes = 0;
    short sequenceNum = 1;

    FILE *fp = NULL;
    fp = fopen(filename, "r");

    if(fp){
      char *buff = calloc(1,BUFFER_SIZE);

      while(fgets(buff,BUFFER_SIZE,fp) != NULL){
        int buffLen = strlen(buff);
        
        //send next data packet
        //send header
        uint16_t headerBuff[2];
        headerBuff[0] = htons(buffLen);
        headerBuff[1] = htons(sequenceNum);
        int bytes_sent1 = send(sock_connection, headerBuff, 4, 0);
        if(bytes_sent1 < 0){
          perror("File header sending error");
          exit(1);
        }

        //send data
        int bytes_sent2 = send(sock_connection, buff, buffLen, 0);
        if(bytes_sent2 < 0){
          perror("File data sending error");
          exit(1);
        }

        //print stats
        printf("Packet %d transmitted with %d data bytes\n", sequenceNum, bytes_sent2);
        totalBytes += bytes_sent2;
        sequenceNum++;

        //clear buffer
        memset(buff, 0, BUFFER_SIZE);
      }
      free(buff);
      fclose(fp);
    }
    else{
      //send file not found message packet
      char notFoundMsg[16] = "FILE NOT FOUND\n";
      short notFoundLen = 16;

      //send data packet
      //send header
      uint16_t headerBuff[2];
      headerBuff[0] = htons(notFoundLen);
      headerBuff[1] = htons(sequenceNum);
      int bytes_sent1 = send(sock_connection, headerBuff, 4, 0);
      if(bytes_sent1 < 0){
        perror("Not found header sending error");
        exit(1);
      }

      //send data
      int bytes_sent2 = send(sock_connection, notFoundMsg, notFoundLen, 0);
      if(bytes_sent1 < 0){
        perror("Not found data sending error");
        exit(1);
      }

      //print stats
      printf("Packet %d transmitted with %d data bytes\n", sequenceNum, bytes_sent2);
      totalBytes += bytes_sent2;
      sequenceNum++;
    }

    //Send end of transmission packet
    //send header
    uint16_t headerBuff[2];
    headerBuff[0] = htons(0);
    headerBuff[1] = htons(sequenceNum);
    int bytes_sent1 = send(sock_connection, headerBuff, 4, 0);
    if(bytes_sent1 < 0){
      perror("End of transmission header sending error");
      exit(1);
    }

    //send null data
    int bytes_sent2 = send(sock_connection, NULL, 0, 0);
    if(bytes_sent1 < 0){
      perror("End of transmission data sending error");
      exit(1);
    }

    //print stats
    printf("End of transmission packet with sequence number %d transmitted with %d data bytes\n", sequenceNum, bytes_sent2);

    //Print total transmission statistics
    printf("\n---Server Statistics---\n");
    printf("Data packets transmitted: %d\n", sequenceNum-1);
    printf("Total number of data bytes transmitted: %d\n", totalBytes);

    //close socket and quit
    close(sock_connection); 
    exit(0);
  }

  /* close the socket */
  close(sock_connection); 
}
