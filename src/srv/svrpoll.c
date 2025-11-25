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
