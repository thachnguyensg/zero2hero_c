#include "parse.h"
#include "common.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int create_db_header(struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    printf("DB header malloc failed\n");
    return STATUS_ERROR;
  }

  header->version = 0x1;
  header->count = 0;
  header->magic = HEADER_MAGIC;
  header->filesize = sizeof(struct dbheader_t);

  *headerOut = header;

  return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0) {
    printf("Bad file description\n");
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
    printf("DB header malloc failed\n");
    return STATUS_ERROR;
  }

  if (read(fd, header, sizeof(struct dbheader_t)) !=
      sizeof(struct dbheader_t)) {
    perror("read");
    free(header);
    return STATUS_ERROR;
  }

  header->version = ntohs(header->version);
  header->magic = ntohl(header->magic);
  header->count = ntohs(header->count);
  header->filesize = ntohl(header->filesize);

  if (header->version != 1) {
    printf("Improper header version\n");
    free(header);
    return STATUS_ERROR;
  }
  if (header->magic != HEADER_MAGIC) {
    printf("Improper header magic (%x)\n", header->magic);
    free(header);
    return STATUS_ERROR;
  }
  // if (header->count != 1) {
  //   printf("Improper header version\n");
  //   free(header);
  //   return STATUS_ERROR;
  // }

  struct stat dbstat = {0};
  if (fstat(fd, &dbstat) == -1) {
    perror("fstat");
    free(header);
    return STATUS_ERROR;
  }

  if (header->filesize != dbstat.st_size) {
    printf("Improper header filesize\n");
    free(header);
    return STATUS_ERROR;
  }

  *headerOut = header;

  return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *header,
                   struct employee_t **employeesOut) {
  if (fd < 0) {
    printf("Bad file description\n");
    return STATUS_ERROR;
  }

  int count = header->count;
  struct employee_t *employees = calloc(count, sizeof(struct employee_t));
  if (employees == NULL) {
    printf("Employees malloc failed\n");
    return STATUS_ERROR;
  }

  if (read(fd, employees, count * sizeof(struct employee_t)) == -1) {
    printf("Failed to read employees\n");
    free(employees);
    return STATUS_ERROR;
  }

  int i;
  for (i = 0; i < count; i++) {
    employees[i].hours = ntohl(employees[i].hours);
  }

  *employeesOut = employees;

  return STATUS_SUCCESS;
}

int add_employee(int fd, struct dbheader_t *header,
                 struct employee_t *employees, char *addstring) {
  if (fd < 0) {
    printf("Bad file description\n");
    return STATUS_ERROR;
  }

  char *name = strtok(addstring, ",");
  char *address = strtok(NULL, ",");
  char *hours = strtok(NULL, ",");

  printf("Adding employee: %s, %s, %s\n", name, address, hours);

  unsigned int hours_int = (unsigned int)atoi(hours);

  return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *header,
                struct employee_t *employees) {
  if (fd < 0) {
    printf("Bad file description\n");
    return STATUS_ERROR;
  }

  header->version = htons(header->version);
  header->count = htons(header->count);
  header->magic = htonl(header->magic);
  header->filesize = htonl(header->filesize);

  if (lseek(fd, 0, SEEK_SET) == -1) {
    perror("lseek");
    return STATUS_ERROR;
  }

  if (write(fd, header, sizeof(struct dbheader_t)) == -1) {
    printf("Failed to write db header\n");
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}
