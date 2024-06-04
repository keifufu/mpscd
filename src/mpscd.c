#define THREAD_IMPLEMENTATION
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static char* get_socket_path(char* name)
{
  static char socket_path[64] = "";
  static bool initialized = false;

  if (!initialized) {
    const char* xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (xdg_runtime_dir == NULL) {
      fprintf(stderr, "XDG_RUNTIME_DIR environment variable not set\n");
      exit(EXIT_FAILURE);
    }

    snprintf(socket_path, 64, "%s/mpscd-%s.sock", xdg_runtime_dir, name);
    initialized = true;
  }

  return socket_path;
}

int messages = 0;

int handle_client(void* data)
{
  int client_fd = *((int*)data);

  char message[512];
  if (recv(client_fd, message, sizeof(message), 0) <= 0) {
    return 0;
  } else {
    char rel_message[555];
    snprintf(rel_message, 555, "%d %s", messages++, message);
    printf("%s\n", rel_message);
  }
  return 0;
}

bool connect_to_consumer(int* client_fd, char* id)
{
  struct sockaddr_un server_addr;
  int client_fd_;

  client_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
  if (client_fd_ == -1) {
    return false;
  }

  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, get_socket_path(id));

  if (connect(client_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    return false;
  }

  if (client_fd != NULL) {
    *client_fd = client_fd_;
  }
  return true;
}

void print_help()
{
  printf("Usage: mpscd consume <id>\n");
  printf("Usage: mpscd produce <id> <message>\n");
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    print_help();
    exit(EXIT_FAILURE);
  }

  if (strcmp(argv[1], "consume") == 0) {
    char* id = argv[2];

    if (connect_to_consumer(NULL, id)) {
      printf("%s is already being consumed\n", id);
      exit(EXIT_FAILURE);
    }

    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
      perror("Failed to create a socket");
      exit(EXIT_FAILURE);
    }

    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, get_socket_path(id));
    unlink(get_socket_path(id));

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
      perror("Failed to bind the socket");
      exit(EXIT_FAILURE);
    }

    listen(server_fd, 5);

    while (true) {
      int len = sizeof(client_addr);
      client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&len);
      if (client_fd == -1) {
        perror("Failed to accept client");
        exit(EXIT_FAILURE);
      }

      handle_client(&client_fd);
    }

    close(server_fd);
  } else if (strcmp(argv[1], "produce") == 0) {
    if (argc < 4) {
      print_help();
      exit(EXIT_FAILURE);
    }
    char* id = argv[2];

    int client_fd;
    if (!connect_to_consumer(&client_fd, id)) {
      printf("No listener running with id %s\n", id);
      exit(EXIT_FAILURE);
    }

    int len = 1;
    for (int i = 1; i < argc; i++) {
      len += strlen(argv[i]) + 1;
    }
    char message[len];
    strcpy(message, argv[3]);
    for (int i = 4; i < argc; i++) {
      strcat(message, " ");
      strcat(message, argv[i]);
    }

    if (send(client_fd, message, sizeof(message), 0) == -1) {
      printf("No listener running with id %s\n", id);
      exit(EXIT_FAILURE);
    }
  } else {
    print_help();
    exit(EXIT_FAILURE);
  }
}