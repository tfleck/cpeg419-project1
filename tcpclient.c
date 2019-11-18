/* tcp_ client.c */ 
/* Programmed by Theo Fleck */
/* October 23, 2019 */     

#include <stdio.h>          /* for standard I/O functions */
#include <stdlib.h>         /* for exit */
#include <string.h>         /* for memset, memcpy, and strlen */
#include <netdb.h>          /* for struct hostent and gethostbyname */
#include <sys/socket.h>     /* for socket, connect, send, and recv */
#include <netinet/in.h>     /* for sockaddr_in */
#include <unistd.h>         /* for close */

#define STRING_SIZE 1024

#define SERVER_HOSTNAME "127.0.0.1"
#define SERVER_PORT 6680

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
  int sock_client;  /* Socket used by client */

  struct sockaddr_in server_addr;  /* Internet address structure that
                                      stores server address */
  struct hostent * server_hp;      /* Structure to store server's IP
                                      address */

  /* open a socket */

  if ((sock_client = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("Client: can't open stream socket");
    exit(1);
  }

  /* Note: there is no need to initialize local client address information 
          unless you want to specify a specific local port
          (in which case, do it the same way as in udpclient.c).
          The local address initialization and binding is done automatically
          when the connect function is called later, if the socket has not
          already been bound. */

  /* initialize server address information */

  if ((server_hp = gethostbyname(SERVER_HOSTNAME)) == NULL) {
    perror("Client: invalid server hostname");
    close(sock_client);
    exit(1);
  }

  /* Clear server address structure and initialize with server address */
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy((char *)&server_addr.sin_addr, server_hp->h_addr,
         server_hp->h_length);
  unsigned short server_port = SERVER_PORT;
  server_addr.sin_port = htons(server_port);

  /* connect to the server */

  if (connect(sock_client, (struct sockaddr *) &server_addr, 
              sizeof (server_addr)) < 0) {
    perror("Client: can't connect to server");
    close(sock_client);
    exit(1);
  }

  //get filename from user
  char filename[STRING_SIZE];
  printf("Please input a filename:\n");
  scanf("%s", filename);
  int msg_len = strlen(filename) + 1;
  printf("\n");

  //send filename packet
  //send header
  uint16_t headerBuff[2];
  headerBuff[0] = htons(msg_len);
  headerBuff[1] = htons(1);
  int bytes_sent = send(sock_client, headerBuff, 4, 0);
  if(bytes_sent < 0){
    perror("Filename header sending error");
    exit(1);
  }

  //send data
  bytes_sent = send(sock_client, filename, msg_len, 0);
  if(bytes_sent < 0){
    perror("Filename data sending error");
    exit(1);
  }

  //initialize variables used in receive loop
  int totalBytes = 0;
  int numPackets = 0;
  int notEot = 1;
  FILE *fp = fopen("out.txt", "wb");

  //Receive in a loop until end of transmission received
  while(notEot){
    //receive next data packet
    //receive header
    char socketMsg1[4];
    memset(&socketMsg1, 0, 4);
    int bytes_recd1 = recv(sock_client, socketMsg1, 4, 0);
    if(bytes_recd1 < 0){
      perror("File header receive error");
      exit(1);
    }

    //decode header
    short count = getShortFromChars(socketMsg1, 0, 1);
    short sequenceNum = getShortFromChars(socketMsg1, 2, 3);

    //receive data
    char socketMsg2[count+1]; //space for null terminator
    memset(&socketMsg2,0,count+1); //ensure buffer is blank
    int bytes_recd2 = recv(sock_client, socketMsg2, sizeof(socketMsg2)-1, 0);
    socketMsg2[count+1] = '\0';
    if(bytes_recd2 < 0){
      perror("File data receive error");
      exit(1);
    }

    //Exit loop if end of transmission
    if(count == 0 && bytes_recd2 == 0){
      printf("End of Transmission Packet with sequence number %d received with %d data bytes\n", sequenceNum, bytes_recd2);
      notEot = 0;
    }
    else{
      //write received data to file out.txt
      if(fp){
        fprintf(fp, "%s", socketMsg2);
      }

      //print stats
      printf("Packet %d received with %d data bytes\n", sequenceNum, bytes_recd2);
      numPackets = sequenceNum;
      totalBytes += bytes_recd2;
    }
  }
  if(fp){
    fclose(fp); 
  }

  //print total transmission statistics
  printf("\n---Client Statistics---\n");
  printf("Number of data packets received: %d\n", numPackets);
  printf("Total number of data bytes received: %d\n", totalBytes);

  /* close the socket */
  close(sock_client);
}
