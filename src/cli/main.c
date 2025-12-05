#include "common.h"
#include "parse.h"
#include "svrpoll.h"
#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int send_hello(int fd);
int send_employee(int fd, char *addstr);
int list_employees_command(int fd);

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
  char *addarg = NULL;
  char *portarg = NULL;
  char *hostarg = NULL;
  unsigned short port = 0;

  bool addcmd = false;
  bool listcmd = false;

  int c;
  while ((c = getopt(argc, argv, "p:h:a:l")) != -1) {
    switch (c) {
    case 'p':
      portarg = optarg;
      port = atoi(portarg);
      break;
    case 'h':
      hostarg = optarg;
      break;
    case 'a':
      addcmd = true;
      addarg = optarg;
      break;
    case 'l':
      listcmd = true;
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
  if (send_hello(fd) == STATUS_ERROR) {
    close(fd);
    return -1;
  }

  if (addcmd) {
    printf("Adding employee: %s\n", addarg);
    if (send_employee(fd, addarg) == STATUS_ERROR) {
      close(fd);
      return -1;
    }
  }

  if (listcmd) {
    printf("Listing employees\n");
    if (list_employees_command(fd) == STATUS_ERROR) {
      close(fd);
      return -1;
    }
  }

  close(fd);

  return 0;
}

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

int send_employee(int fd, char *addstr) {
  char buf[4096] = {0};

  dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
  hdr->type = MSG_EMPLOYEE_ADD_REQ;
  hdr->len = 1;

  dbproto_employee_add_req_t *addreq = (dbproto_employee_add_req_t *)&hdr[1];
  strncpy((char *)addreq->data, addstr, sizeof(addreq->data));

  hdr->type = htonl(hdr->type);
  hdr->len = htons(hdr->len);

  if (send(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_employee_add_req_t),
           0) == -1) {
    perror("send");
    return STATUS_ERROR;
  }

  if (recv(fd, buf, sizeof(buf), 0) == -1) {
    perror("recv");
    return STATUS_ERROR;
  }

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (hdr->type == MSG_ERROR) {
    printf("Error adding employee\n");
    return STATUS_ERROR;
  }

  if (hdr->type != MSG_EMPLOYEE_ADD_RESP) {
    printf("Unexpected response type: %d\n", hdr->type);
    return STATUS_ERROR;
  }

  printf("Employee added successfully\n");
  return STATUS_SUCCESS;
}

int list_employees_command(int fd) {
  char buf[4096] = {0};

  dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
  hdr->type = MSG_EMPLOYEE_LIST_REQ;
  hdr->len = 0;

  hdr->type = htonl(hdr->type);
  hdr->len = htons(hdr->len);

  if (send(fd, buf, sizeof(dbproto_hdr_t), 0) == -1) {
    perror("send");
    return STATUS_ERROR;
  }

  // receive response header
  if (recv(fd, buf, sizeof(dbproto_hdr_t), 0) == -1) {
    perror("recv");
    return STATUS_ERROR;
  }

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (hdr->type == MSG_ERROR) {
    printf("Error adding employee\n");
    return STATUS_ERROR;
  }

  if (hdr->type != MSG_EMPLOYEE_LIST_RESP) {
    printf("Unexpected response type: %d / %d\n", hdr->type,
           MSG_EMPLOYEE_LIST_RESP);
    return STATUS_ERROR;
  }

  printf("Listing %d employee...\n", hdr->len);
  uint16_t recv_employees = 0;
  uint16_t expected_employees = hdr->len;

  // receive each employee one by one
  while (recv_employees < expected_employees) {
    if (recv(fd, buf,
             sizeof(dbproto_hdr_t) + sizeof(dbproto_employee_list_resp_t),
             0) == -1) {
      perror("recv");
      return STATUS_ERROR;
    }

    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (hdr->type != MSG_EMPLOYEE_LIST_RESP) {
      printf("Unexpected response type: %d\n", hdr->type);
      return STATUS_ERROR;
    }

    dbproto_employee_list_resp_t *resp =
        (dbproto_employee_list_resp_t *)&hdr[1];

    struct employee_t employee = resp->employee;
    employee.hours = ntohl(employee.hours);

    printf("Received employee %d/%d: ", recv_employees + 1, expected_employees);
    printf("%s, %s, %d\n", employee.name, employee.address, employee.hours);

    recv_employees++;
  }

  return STATUS_SUCCESS;
}
