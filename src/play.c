#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_USERNAME_SIZE 32
#define MAX_HOST_SIZE 256

int main() {
  char username[MAX_USERNAME_SIZE];
  char host[MAX_HOST_SIZE];

  printf("=== Pokemon Battle ===\n\n");

  printf("Enter server hostname: ");
  fflush(stdout);
  if (fgets(host, MAX_HOST_SIZE, stdin) == NULL) {
    fprintf(stderr, "Error reading hostname.\n");
    exit(1);
  }
  host[strcspn(host, "\n")] = '\0';
  if (strlen(host) == 0) {
    fprintf(stderr, "Hostname cannot be empty.\n");
    exit(1);
  }

  printf("Enter your username: ");
  fflush(stdout);
  if (fgets(username, MAX_USERNAME_SIZE, stdin) == NULL) {
    fprintf(stderr, "Error reading username.\n");
    exit(1);
  }
  username[strcspn(username, "\n")] = '\0';
  if (strlen(username) == 0) {
    fprintf(stderr, "Username cannot be empty.\n");
    exit(1);
  }

  execl("./build/client", "client", host, username, NULL);

  perror("execl");
  return 1;
}
