#include "http_message.h"
#include "request_endpoints.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define LISTEN_BACKLOG 5

int request_count = 0;
int sent_bytes = 0;
int received_bytes = 0;

//takes in the clients message and see which enpoint they are trying to access
int respond_to_http_client_message(int client_socket,
                                   http_client_message_t *message) {

  if (!strcmp(message->method, "GET")) {

    if (!strncmp(message->path, "/calc/", 6)) {
      fprintf(stderr, "%d", message->body_length);
      int sent = calc_endpoint(client_socket, message->path);
      request_count++;
      sent_bytes += sent;
    } else if (!strncmp(message->path, "/stats", 6)) {
      fprintf(stderr, "stats endpoint\n");
      int sent = stats_endpoint(client_socket, message->path, request_count,
                                sent_bytes, received_bytes);
      request_count++;
      sent_bytes += sent;
    } else if (!strncmp(message->path, "/static/images/", 15)) {
      fprintf(stderr, "images endpoint\n");
      int sent = images_endpoint(client_socket, message->path);
      request_count++;
      sent_bytes += sent;
    } else if (!strncmp(message->path, "/random", 7)) {
      fprintf(stderr, "random endpoint\n");
      int sent = random_endpoint(client_socket, message->path);
      request_count++;
      sent_bytes += sent;

    } else {
      char err_response[1024];
      int response_length =
          snprintf(err_response, sizeof(err_response),
                   "<html><body><h1>404 Not Found</h1></body></html>\n");

      write(client_socket, err_response, response_length);
    }
  } else {
    char err_response[1024];
    int response_length =
        snprintf(err_response, sizeof(err_response),
                 "<html><body><h1>405 Method Not Allowed</h1></body></html>\n");

    write(client_socket, err_response, response_length);
  }
  return 0;
}

//handles the connections and reads the message and sends it to the endpoint
void *handleConnection(void *client_socket_ptr) {
  fprintf(stderr, "handling connection\n");
  int client_socket = *((int *)client_socket_ptr);
  free(client_socket_ptr);

  while (1) {
    http_client_message_t *http_msg = NULL;
    http_read_result_t result;

    read_http_client_message(client_socket, &http_msg, &result,
                             &received_bytes);
    if (result == CLOSED_CONNECTION) {
      fprintf(stderr, "Connection closed\n");
      close(client_socket);
      return NULL;
    }
    if (result == BAD_REQUEST) {
      fprintf(stderr, "Bad request\n");
      close(client_socket);
      return NULL;
    }
    if (result == MESSAGE) {
      fprintf(stderr, "Message received\n");
      fprintf(stderr,
              "Received HTTP message: method=%s, path=%s, http_version=%s, "
              "body=%s\n",
              http_msg->method, http_msg->path, http_msg->http_version,
              http_msg->body ? http_msg->body : "NULL");

      respond_to_http_client_message(client_socket, http_msg);
    }

    if (http_msg) {
      http_client_message_free(http_msg);
    }
  }
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
    printf("Accepted connection\n");

    int *client_socket_ptr = malloc(sizeof(int));
    *client_socket_ptr = client_socket;
    printf("Creating thread\n");
    pthread_t thread_id;
    int thread_result = pthread_create(&thread_id, NULL, handleConnection,
                                       (void *)client_socket_ptr);

    if (thread_result != 0) {
      perror("Thread creation failed");
      close(client_socket);
      free(client_socket_ptr);
      continue;
    }
  }

  close(socket_fd);
  return 0;
}