#include "connect.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

void handle_server(int fd) {
  char buf[4096] = {0};
  int nbytes = recv(fd, buf, sizeof(buf) - 1, 0);
  if (nbytes == -1) {
    perror("recv");
    close(fd);
  }

  proto_header_t *header = (proto_header_t *)buf;
  int *data = (int *)&header[1];
  header->type = ntohl(header->type);
  header->length = ntohs(header->length);
  *data = ntohl(*data);

  if (header->type != PROTO_HELLO) {
    printf("Protocol mismatch");
    return;
  }

  if (*data != 1) {
    printf("Protocol version mismatch\n");
    return;
  }

  printf("Received message type: %d, length: %d, data: %d\n", header->type,
         header->length, *data);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <server_ip>\n", argv[0]);
    return -1;
  }

  struct sockaddr_in server_info = {0};
  socklen_t server_size = sizeof(server_info);

  server_info.sin_family = AF_INET;
  server_info.sin_addr.s_addr = inet_addr(argv[1]);
  server_info.sin_port = htons(9090);

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

  handle_server(fd);

  close(fd);

  return 0;
}
