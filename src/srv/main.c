#include "common.h"
#include "file.h"
#include "parse.h"
#include "svrpoll.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

clientstate_t clientstates[MAX_CLIENTS];

int poll_loop(short port);

void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <database file>\n", argv[0]);
  printf("\t -n - create new database file\n");
  printf("\t -f - (required) path to database file\n");
  printf("\t -a - add employee via CSV (name,address,hours)\n");
  printf("\t -l - list the employees\n");
  printf("\t -r - remove employee by name\n");
}

int main(int argc, char *argv[]) {

  bool newfile = false;
  char *filepath = NULL;
  char *addstring = NULL;
  bool listflag = false;
  char *removestring = NULL;
  char *updatestring = NULL;
  char *updatehours = NULL;
  char *adrstring = NULL;
  char *namestring = NULL;
  unsigned short server_port = PORT;
  int dbfd;
  struct dbheader_t *header = NULL;
  struct employee_t *employees = NULL;
  int c;

  while ((c = getopt(argc, argv, "nf:p:a:lr:u:i:h:w:")) != -1) {
    switch (c) {
    case 'n':
      newfile = true;
      break;
    case 'f':
      filepath = optarg;
      break;
    case 'p':
      server_port = (unsigned short)atoi(optarg);
      break;
    case 'a':
      addstring = optarg;
      break;
    case 'l':
      listflag = true;
      break;
    case 'r':
      removestring = optarg;
      break;
    case 'u':
      updatestring = optarg;
      break;
    case 'i':
      namestring = optarg;
      break;
    case 'h':
      updatehours = optarg;
      break;
    case 'w':
      adrstring = optarg;
      break;
    case '?':
      printf("Unknown option - %c\n", c);
      break;
    default:
      return -1;
    }
  }

  if (filepath == NULL) {
    printf("Filepath is required\n");
    print_usage(argv);
    return 0;
  }

  if (newfile) {
    dbfd = create_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to create database file\n");
      return -1;
    }

    if (create_db_header(&header) != STATUS_SUCCESS) {
      printf("Failed to create database header\n");
      return -1;
    }
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to open database file\n");
      return -1;
    }

    if (validate_db_header(dbfd, &header) == STATUS_ERROR) {
      printf("Invalid database header\n");
      return -1;
    }
  }

  if (read_employees(dbfd, header, &employees) == STATUS_ERROR) {
    printf("Failed to read employees from database file\n");
    return -1;
  }

  if (addstring) {
    printf("Adding employee: %s\n", addstring);
    if (add_employee(header, &employees, addstring) == STATUS_ERROR) {
      printf("Failed to add employee: %s\n", addstring);
      return -1;
    }
  }

  if (removestring) {
    printf("Removing employee name: %s\n", removestring);
    if (remove_employee_by_name(header, employees, removestring) ==
        STATUS_ERROR) {
      printf("Failed to remove employee name: %s\n", removestring);
      return -1;
    }
  }

  if (updatestring) {
    struct employee_t newdata = {0};
    int updateflags = 0;

    printf("Updating employee %s \n", updatestring);
    if (updatehours) {
      printf("Updating hours to %s\n", updatehours);
      newdata.hours = (unsigned int)atoi(updatehours);
      updateflags |= UPDATE_HOURS;
    }
    if (namestring) {
      printf("Updating name to %s\n", namestring);
      strncpy(newdata.name, namestring, sizeof(newdata.name));
      updateflags |= UPDATE_NAME;
    }
    if (adrstring) {
      printf("Updating address to %s\n", adrstring);
      strncpy(newdata.address, adrstring, sizeof(newdata.address));
      updateflags |= UPDATE_ADDRESS;
    }

    if (update_employee(header, employees, updatestring, &newdata,
                        updateflags) == STATUS_ERROR) {
      printf("Failed to update employee %s \n", updatestring);
      return -1;
    }
  }

  printf("number of employee: %d\n", header->count);

  if (listflag) {
    list_employees(header, employees);
  }

  poll_loop(server_port);

  if (output_file(dbfd, header, employees) == STATUS_ERROR) {
    printf("Failed to write database file\n");
    return -1;
  }

  free(header);
  free(employees);

  return 0;
}

int poll_loop(short port) {
  int listen_fd, conn_fd, free_slot;
  struct sockaddr_in server_addr, client_addr;
  socklen_t client_size = sizeof(client_addr);

  struct pollfd fds[MAX_CLIENTS + 1];
  nfds_t nfds = 1;
  int opt = 1;

  init_clients(clientstates);

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    perror("socket");
    return -1;
  }

  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    perror("setsockopt");
    close(listen_fd);
    return -1;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("bind");
    close(listen_fd);
    return -1;
  }

  if (listen(listen_fd, BACKLOG) == -1) {
    perror("listen");
    close(listen_fd);
    return -1;
  }

  printf("Server listening on port %d\n", port);

  memset(fds, 0, sizeof(fds));
  fds[0].fd = listen_fd;
  fds[0].events = POLLIN;
  nfds = 1;

  while (1) {
    // Add client sockets to the read set
    int current_nfds = 1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clientstates[i].fd != -1) {
        fds[current_nfds].fd = clientstates[i].fd;
        fds[current_nfds].events = POLLIN;
        current_nfds++;
      }
    }

    // Wait for activity on any socket
    int events = poll(fds, nfds, -1);
    if (events < 0) {
      perror("poll");
      return -1;
    }

    // Check for new incoming connections
    if (fds[0].revents & POLLIN) {
      if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr,
                            &client_size)) == -1) {
        perror("accept");
        continue;
      }

      printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));

      free_slot = find_free_slot(clientstates);
      if (free_slot == -1) {
        printf("Max clients reached. Connection rejected.\n");
        close(conn_fd);
      } else {
        clientstates[free_slot].fd = conn_fd;
        clientstates[free_slot].state = STATE_CONNECTED;
        nfds++;
        printf("Client with fd %d added in slot %d\n", conn_fd, free_slot);
      }
      events--;
    }

    for (int i = 1; i <= nfds && events > 0; i++) {
      if (fds[i].revents & POLLIN) {
        events--;
        // Handle client communication
        int fd = fds[i].fd;
        int slot = find_slot_by_fd(clientstates, fd);
        ssize_t bytes_received = read(fd, clientstates[slot].buffer,
                                      sizeof(clientstates[slot].buffer));

        if (bytes_received <= 0) {
          close(fd);
          if (slot == -1) {
            printf("close fd that does not exist\n");
            continue;
          }
          clientstates[slot].fd = -1;
          clientstates[slot].state = STATE_DISCONNECTED;
          printf("Client disconnected or error\n");
          nfds--;
        } else {
          // clientstates[i].buffer[bytes_received] = '\0';
          printf("Received data from client: %s\n", clientstates[slot].buffer);
        }
      }
    }
  }

  close(listen_fd);

  return -1;
}

void handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t *employees,
                       clientstate_t *client) {
  dbproto_hdr_t *hdr = (dbproto_hdr_t *)client->buffer;

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  if (client->state == STATE_HELLO) {
    if (hdr->type != MSG_HELLO_REQ && hdr->len != 1) {
      printf("Invalid HELLO message\n");
      return;
    }

    dbproto_hello_req_t *hello_req = (dbproto_hello_req_t *)&hdr[1];
    hello_req->proto = ntohs(hello_req->proto);
    if (hello_req->proto != PROTO_VERSION) {
      printf("Unsupported protocol version: %d\n", hello_req->proto);
      return;
    }

    client->state = STATE_MSG;
  }

  if (client->state == STATE_MSG) {
  }
}
