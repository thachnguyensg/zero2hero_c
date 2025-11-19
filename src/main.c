#include "common.h"
#include "file.h"
#include "parse.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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
  int dbfd;
  struct dbheader_t *header = NULL;
  struct employee_t *employees = NULL;
  int c;

  while ((c = getopt(argc, argv, "nf:a:lr:")) != -1) {
    switch (c) {
    case 'n':
      newfile = true;
      break;
    case 'f':
      filepath = optarg;
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

  printf("number of employee: %d\n", header->count);

  if (listflag) {
    list_employees(header, employees);
  }

  if (output_file(dbfd, header, employees) == STATUS_ERROR) {
    printf("Failed to write database file\n");
    return -1;
  }

  free(header);
  free(employees);

  return 0;
}
