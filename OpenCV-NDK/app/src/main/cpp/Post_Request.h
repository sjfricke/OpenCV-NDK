// TODO - Not make this file look like crap
// Get rid of string warning too
#ifndef OPENCV_NDK_POST_REQUEST_H
#define OPENCV_NDK_POST_REQUEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>

#include <android/log.h>
#define LOG_TAG2 "POST_REQUEST"
#define LOGI2(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG2, __VA_ARGS__)

#define MSG_SIZE 2048


static void jumpingJackPost(int jacks, int time) {

  int mySocket; // holds ID of the socket
  struct sockaddr_in serv; // object of server to connect to
  char* server_reply;
  char hostIP[32] = "10.196.45.177";
  int port = 4000;
  char publicKey[128] = "0xaD07635Bd5ef027C4bbc44AEe2E7679CdE469998";
  char postBody[512];
  char message[MSG_SIZE];
  sprintf(postBody, "{\"publicKey\" : \"%s\",\"exerciseId\" : 200,\"data\" : {\"jacks\" : %d,\"time\" : %d}}", publicKey, jacks, time);
  sprintf(message, "POST /api/contract/submit HTTP/1.1\nHost: 127.0.0.1:4000\nContent-Type: application/json\nContent-Length: %zu\nCache-Control: no-cache\n\n%s\n", strlen(postBody), postBody);

  int status; // used to get function return values

  // allocate space for messages
  server_reply = (char*) malloc(sizeof(char) * MSG_SIZE);
  if (server_reply == NULL) { LOGI2("ERROR allocating server_reply"); }

  // Create socket
  // AF_INET refers to the Internet Domain
  // SOCK_STREAM sets a stream to send data
  // 0 will have the OS pick TCP for SOCK_STREAM
  mySocket = socket(AF_INET, SOCK_STREAM, 0);
  if (mySocket < 0) { LOGI2 ("Could not create socket"); }

  serv.sin_addr.s_addr = inet_addr(hostIP); // sets IP of server
  serv.sin_family = AF_INET; // uses internet address domain
  serv.sin_port = htons(port); // sets PORT on server

  // Connect to remote server with socket
  status = connect(mySocket, (struct sockaddr *)&serv , sizeof(serv));
  if (status < 0) { LOGI2("Connection error"); }

  LOGI2("Connected\n");

  // sends all data of MSG_SIZE bytes
  status =  send(mySocket, message , MSG_SIZE, 0);
  if (status < 0) { LOGI2("Send failed\n"); }

  LOGI2("Sent Message\n");

  // receive a reply from the server
  status = recv(mySocket, server_reply, MSG_SIZE, 0);
  if (status < 0) {
    LOGI2("ERROR: Reply failed\n");
  } else {
    LOGI2("Reply received: %s\n\n", server_reply);
  }
  close(mySocket);
}
#endif //OPENCV_NDK_POST_REQUEST_H