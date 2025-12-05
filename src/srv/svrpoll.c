#include "common.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <svrpoll.h>

void init_clients(clientstate_t *clientstates) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    clientstates[i].fd = -1;
    clientstates[i].state = STATE_NEW;
    memset(clientstates[i].buffer, '\0', BUFFER_SIZE);
  }
}

int find_free_slot(clientstate_t *clientstates) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientstates[i].fd == -1) {
      return i;
    }
  }
  return -1;
}

int find_slot_by_fd(clientstate_t *clientstates, int fd) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientstates[i].fd == fd) {
      return i;
    }
  }
  return -1;
}

void fsm_reply_hello(clientstate_t *client, dbproto_hdr_t *hdr) {
  hdr->type = htonl(MSG_HELLO_RESP);
  hdr->len = htons(1);

  ssize_t bytes_sent =
      send(client->fd, client->buffer, sizeof(dbproto_hdr_t), 0);
  if (bytes_sent == -1) {
    perror("send");
  }
}

void fsm_reply_hello_error(clientstate_t *client, dbproto_hdr_t *hdr) {
  hdr->type = htonl(MSG_ERROR);
  hdr->len = htons(1);

  ssize_t bytes_sent =
      send(client->fd, client->buffer, sizeof(dbproto_hdr_t), 0);
  if (bytes_sent == -1) {
    perror("send");
  }
}

int fsm_reply_employee_add(clientstate_t *client, dbproto_hdr_t *hdr,
                           int status) {
  uint32_t type;
  if (status == STATUS_SUCCESS) {
    type = MSG_EMPLOYEE_ADD_RESP;
    printf("Employee added successfully\n");
  } else {
    type = MSG_ERROR;
    printf("Error adding employee\n");
  }

  hdr->type = htonl(type);
  hdr->len = htons(1);

  ssize_t bytes_sent =
      send(client->fd, client->buffer, sizeof(dbproto_hdr_t), 0);
  if (bytes_sent == -1) {
    perror("send");
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t **employees,
                       clientstate_t *client, int dbfd) {
  dbproto_hdr_t *hdr = (dbproto_hdr_t *)client->buffer;

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (client->state == STATE_CONNECTED) {
    client->state = STATE_HELLO;
  }

  if (client->state == STATE_HELLO) {
    if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
      printf("Invalid HELLO message\n");
      return;
    }

    dbproto_hello_req_t *hello_req = (dbproto_hello_req_t *)&hdr[1];
    hello_req->proto = ntohs(hello_req->proto);
    if (hello_req->proto != PROTO_VERSION) {
      printf("Unsupported protocol version: %d\n", hello_req->proto);
      fsm_reply_hello_error(client, hdr);
      return;
    }

    fsm_reply_hello(client, hdr);

    client->state = STATE_MSG;
    printf("Client upgraded to STATE_MSG\n");
  }

  if (client->state == STATE_MSG) {
    if (hdr->type == MSG_EMPLOYEE_ADD_REQ) {
      printf("Received EMPLOYEE_ADD_REQ\n");
      dbproto_employee_add_req_t *add_req =
          (dbproto_employee_add_req_t *)&hdr[1];

      int status = add_employee(dbhdr, employees, (char *)add_req->data);
      if (fsm_reply_employee_add(client, hdr, status) == STATUS_ERROR) {
        printf("Failed to send EMPLOYEE_ADD_RESP\n");
        return;
      }

      if (status == STATUS_SUCCESS) {
        output_file(dbfd, dbhdr, *employees);
      }

      printf("Sent EMPLOYEE_ADD_RESP\n");
    }
  }
}
