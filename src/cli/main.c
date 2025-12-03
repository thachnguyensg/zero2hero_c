#include "common.h"
#include "svrpoll.h"
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int send_hello(int fd) {
  char buf[4096] = {0};

  dbproto_hdr_t *header = (dbproto_hdr_t *)buf;
  header->type = MSG_HELLO_REQ;
  header->len = 1;

  dbproto_hello_req_t *hello = (dbproto_hello_req_t *)&header[1];
  hello->proto = PROTO_VERSION;

  header->type = htonl(header->type);
  header->len = htons(header->len);
  hello->proto = htons(hello->proto);

  if (write(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req_t)) ==
      -1) {
    perror("write");
    return STATUS_ERROR;
  }

  int n_bytes = recv(fd, buf, sizeof(buf), 0);
  if (n_bytes < 0) {
    perror("recv");
    return STATUS_ERROR;
  }

  header->type = ntohl(header->type);
  header->len = ntohl(header->len);

  if (header->type == MSG_ERROR) {
    printf("Protocol mismatch\n");
    return STATUS_ERROR;
  }

  printf("Server connected, protocol v1.\n");
  return STATUS_SUCCESS;
}

void handle_server(int fd) {
  char buf[4096] = {0};
  int nbytes = recv(fd, buf, sizeof(buf) - 1, 0);
  if (nbytes == -1) {
    perror("recv");
    close(fd);
  }

  dbproto_hdr_t *header = (dbproto_hdr_t *)buf;
  int *data = (int *)&header[1];
  header->type = ntohl(header->type);
  header->len = ntohs(header->len);
  *data = ntohl(*data);

  if (header->type != PROTO_VERSION) {
    printf("Protocol mismatch");
    return;
  }

  if (*data != 1) {
    printf("Protocol version mismatch\n");
    return;
  }

  printf("Received message type: %d, length: %d, data: %d\n", header->type,
         header->len, *data);
}

int main(int argc, char *argv[]) {
  // if (argc != 2) {
  //   printf("Usage: %s <server_ip>\n", argv[0]);
  //   return -1;
  // }

  char *addarg = NULL;
  char *portarg = NULL;
  char *hostarg = NULL;
  unsigned short port = 0;

  int c;
  while ((c = getopt(argc, argv, "p:h:a:")) != -1) {
    switch (c) {
    case 'a':
      addarg = optarg;
      break;
    case 'p':
      portarg = optarg;
      port = atoi(portarg);
      break;
    case 'h':
      hostarg = optarg;
      break;
    case '?':
      printf("Unknonw option: %c\n", c);
    default:
      return EXIT_FAILURE;
    }
  }

  if (port == 0) {
    printf("Bad port: %s'\n", portarg);
    return EXIT_FAILURE;
  }
  if (hostarg == NULL) {
    printf("Must specify host with -h\n");
  }

  struct sockaddr_in server_info = {0};
  socklen_t server_size = sizeof(server_info);

  server_info.sin_family = AF_INET;
  server_info.sin_addr.s_addr = inet_addr(hostarg);
  server_info.sin_port = htons(PORT);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    return -1;
  }

  if (connect(fd, (struct sockaddr *)&server_info, server_size) == -1) {
    perror("connect");
    close(fd);
    return -1;
  }

  printf("Sending hello message\n");
  send_hello(fd);

  // printf("Waiting server repsonse\n");
  // handle_server(fd);

  close(fd);

  return 0;
}
