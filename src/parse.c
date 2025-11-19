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
    printf("Bad file descriptor\n");
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
  header->count = ntohs(header->count);
  header->magic = ntohl(header->magic);
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

int add_employee(struct dbheader_t *header, struct employee_t **employees,
                 char *addstring) {

  if (NULL == header)
    return STATUS_ERROR;
  if (NULL == employees)
    return STATUS_ERROR;
  if (NULL == *employees)
    return STATUS_ERROR;
  if (NULL == addstring)
    return STATUS_ERROR;

  char *name = strtok(addstring, ",");
  if (NULL == name)
    return STATUS_ERROR;
  char *address = strtok(NULL, ",");
  if (NULL == address)
    return STATUS_ERROR;
  char *hours = strtok(NULL, ",");
  if (NULL == hours)
    return STATUS_ERROR;

  struct employee_t *empArray = *employees;
  empArray = realloc(empArray, (header->count + 1) * sizeof(struct employee_t));
  if (empArray == NULL) {
    printf("Realloc failed\n");
    return STATUS_ERROR;
  }

  header->count++;
  strncpy(empArray[header->count - 1].name, name,
          sizeof(empArray[header->count - 1].name));
  strncpy(empArray[header->count - 1].address, address,
          sizeof(empArray[header->count - 1].address));
  empArray[header->count - 1].hours = atoi(hours);

  *employees = empArray;

  return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *header,
                struct employee_t *employees) {
  if (fd < 0) {
    printf("Bad file description\n");
    return STATUS_ERROR;
  }

  int count = header->count;

  header->version = htons(header->version);
  header->count = htons(header->count);
  header->magic = htonl(header->magic);
  header->filesize =
      htonl(sizeof(struct dbheader_t) + (count * sizeof(struct employee_t)));

  if (lseek(fd, 0, SEEK_SET) == -1) {
    perror("lseek");
    return STATUS_ERROR;
  }

  if (write(fd, header, sizeof(struct dbheader_t)) == -1) {
    printf("Failed to write db header\n");
    return STATUS_ERROR;
  }

  int i;
  for (i = 0; i < count; i++) {
    employees[i].hours = htonl(employees[i].hours);
    write(fd, &employees[i], sizeof(struct employee_t));
  }

  return STATUS_SUCCESS;
}

void list_employees(struct dbheader_t *header, struct employee_t *employees) {
  if (NULL == header)
    return;
  if (NULL == employees)
    return;
  int i;
  for (i = 0; i < header->count; i++) {
    printf("Employee %d:\n", i + 1);
    printf("\tName: %s\n", employees[i].name);
    printf("\tAddress: %s\n", employees[i].address);
    printf("\tHours: %d\n", employees[i].hours);
  }
}
