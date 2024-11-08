#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define LISTEN_BACKLOG 5

int request_count = 0;
int sent_bytes = 0;
int received_bytes = 0;

void *handleConnection(void *client_socket_ptr) {
  int client_socket = *((int *)client_socket_ptr);
  free(client_socket_ptr);

  char buffer[1024];
  int valread = read(client_socket, buffer, sizeof(buffer));

  printf("Received: %s\n", buffer);
  char path[1000];
  char method[1000];
  char http_version[1000];
  sscanf(buffer, "%s %s %s", method, path, http_version);

  if (!strcmp(method, "GET") && !strncmp(path, "/calc/", 6)) {
    int a, b;
    if (sscanf(path, "/calc/%d/%d", &a, &b) == 2) {
      char response[1024];
      int response_length =
          snprintf(response, sizeof(response),
                   "<html><body><h1>Sum: %d</h1></body></html>\n", a + b);

      sent_bytes += response_length;
      received_bytes += valread;
      request_count++;
      write(client_socket, response, response_length);
    } else {
      char err_response[1024];
      int response_length =
          snprintf(err_response, sizeof(err_response),
                   "<html><body><h1>404 Not Found</h1></body></html>\n");

      write(client_socket, err_response, response_length);
    }
  }

  if (!strcmp(method, "GET") && !strncmp(path, "/stats/", 7)) {
    char response[2048];

    int response_length = snprintf(response, sizeof(response),
                                   "<html><body><h1>Stats</h1>"
                                   "<p>Request count: %d</p>"
                                   "<p>Sent bytes: %d</p>"
                                   "<p>Received bytes: %d</p></body></html>\n",
                                   request_count, sent_bytes, received_bytes);

    sent_bytes += response_length;
    received_bytes += valread;
    request_count++;
    write(client_socket, response, response_length);
  }

  if (!strcmp(method, "GET") && !strncmp(path, "/static/images/", 15)) {
    char file[1000];
    if (sscanf(path, "/static/images/%999s", file) == 1) {
      int fd = open(file, O_RDONLY);
      if (fd != -1) {
        struct stat file_stat;
        fstat(fd, &file_stat);
        char *file_content = malloc(file_stat.st_size);
        read(fd, file_content, file_stat.st_size);
        close(fd);

        char response_header[1024];
        int response_header_length =
            snprintf(response_header, sizeof(response_header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Length: %ld\r\n"
                     "Content-Type: application/octet-stream\r\n"
                     "\r\n",
                     file_stat.st_size);
        write(client_socket, response_header, response_header_length);
        write(client_socket, file_content, file_stat.st_size);

        sent_bytes += response_header_length + file_stat.st_size;
        received_bytes += valread;
        request_count++;
        free(file_content);
      }
    } else {
      char err_response[1024];
      int response_length =
          snprintf(err_response, sizeof(err_response),
                   "<html><body><h1>404 Not Found</h1></body></html>\n");

      write(client_socket, err_response, response_length);
    }
  }
  close(client_socket);
  return NULL;
}

int main(int argc, char *argv[]) {

  int port = 80;

  if (argc > 1) {
    if ((strcmp(argv[1], "-p")) == 0) {
      if (argc == 3 && atoi(argv[2]) > 1023) {
        port = atoi(argv[2]);
      }
      printf("Listening to port %d\n", port);
    }
  }

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in socket_address;
  memset(&socket_address, '\0', sizeof(socket_address));
  socket_address.sin_family = AF_INET;
  socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
  socket_address.sin_port = htons(port);

  int returnval;

  returnval = bind(socket_fd, (struct sockaddr *)&socket_address,
                   sizeof(socket_address));
    if (returnval == -1) {
    perror("Bind failed");
    return 1;
    }
    

  returnval = listen(socket_fd, LISTEN_BACKLOG);

  struct sockaddr_in client_address;
  socklen_t client_address_len = sizeof(client_address);

  while (1) {
    int client_socket = accept(socket_fd, (struct sockaddr *)&client_address,
                               &client_address_len);
    if (client_socket == -1) {
      perror("Accept failed");
      return 1;
    } 

    int *client_socket_ptr = malloc(sizeof(int));
    *client_socket_ptr = client_socket;

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, handleConnection,
                   (void *)client_socket_ptr);
  }
  close(socket_fd);
  return 0;
}