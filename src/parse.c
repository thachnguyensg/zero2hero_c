#include "parse.h"
#include "common.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
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
    printf("Improper header magic\n");
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
  // TODO: Implement this function to read employee records from the database
  // file
  return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *header,
                struct employee_t *employees) {
  if (fd < 0) {
    printf("Bad file descriptor\n");
    return STATUS_ERROR;
  }
  struct dbheader_t temp_header = {0};

  temp_header.version = htons(header->version);
  temp_header.count = htons(header->count);
  temp_header.magic = htonl(header->magic);
  temp_header.filesize = htonl(header->filesize);

  if (lseek(fd, 0, SEEK_SET) == -1) {
    perror("lseek");
    return STATUS_ERROR;
  }

  if (write(fd, &temp_header, sizeof(struct dbheader_t)) !=
      sizeof(struct dbheader_t)) {
    printf("Failed to write db header\n");
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}
