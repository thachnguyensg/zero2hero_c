#include "common.h"
#include <netinet/in.h>
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

void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t *employees,
                       clientstate_t *client) {
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
  }
}
