#ifndef SVRPOLL_H
#define SVRPOLL_H

#include "parse.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096

#define PORT 8080
#define BACKLOG 5

typedef enum {
  STATE_NEW,
  STATE_CONNECTED,
  STATE_DISCONNECTED,
  STATE_HELLO,
  STATE_MSG,
  STATE_GOODBYE,
} state_e;

typedef struct {
  int fd;
  state_e state;
  char buffer[BUFFER_SIZE];
} clientstate_t;

void init_clients(clientstate_t *clientstates);
int find_free_slot(clientstate_t *clientstates);
int find_slot_by_fd(clientstate_t *clientstates, int fd);
void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t **employees,
                       clientstate_t *client, int dbfd);

#endif // !SVRPOLL_H
