
#include "http_message.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

bool is_complete_http_message(char *buffer) {
  if (strlen(buffer) < 10) {
    return false;
  }
  if (strncmp(buffer, "GET ", 4) != 0) {
    return false;
  }
  // Check if the message has ended from a web browser or telnet
  if (strstr(buffer, "\r\n\r\n") != NULL || strstr(buffer, "\n\n") != NULL) {
    return true;
  }
  return false;
}

void read_http_client_message(int client_socket, http_client_message_t **msg,
                              http_read_result_t *result, int *recieved_bytes) {
  fprintf(stderr, "reading message\n");
  *msg = malloc(sizeof(http_client_message_t));
  if (*msg == NULL) {
    *result = BAD_REQUEST;
    return;
  }
  char buffer[4096];
  strcpy(buffer, "");

  while (!is_complete_http_message(buffer)) {
    int valread = read(client_socket, buffer + strlen(buffer),
                       sizeof(buffer) - strlen(buffer) - 1);
    if (valread == 0) {
      *result = CLOSED_CONNECTION;
      return;
    }
    if (valread < 0) {
      *result = BAD_REQUEST;
      return;
    }
    buffer[valread + strlen(buffer)] = '\0';
  }
  *recieved_bytes += strlen(buffer);
  char *method = strtok(buffer, " ");
  char *path = strtok(NULL, " ");
  char *http_version = strtok(NULL, "\r\n");

  if (method == NULL || path == NULL || http_version == NULL) {
    *result = BAD_REQUEST;
    return;
  }

  (*msg)->method = strdup(method);
  (*msg)->path = strdup(path);
  (*msg)->http_version = strdup(http_version);

  if (strcmp((*msg)->path, "/favicon.ico") == 0) {
    *result = BAD_REQUEST;
    return;
  }
  // Find the body
  char *headers_end = strstr(buffer, "\r\n\r\n");
  if (headers_end) {
    headers_end += 4; // Move past the "\r\n\r\n"
    (*msg)->headers = strndup(buffer, headers_end - buffer);
  } else {
    (*msg)->headers = NULL;
  }

  // Parse body
  if (headers_end && *headers_end != '\0') {
    (*msg)->body = strdup(headers_end);
    (*msg)->body_length = strlen(headers_end);
  } else {
    (*msg)->body = NULL;
    (*msg)->body_length = 0;
  }

  *result = MESSAGE;
}

void http_client_message_free(http_client_message_t *msg) {
  free(msg->method);
  free(msg->path);
  free(msg->http_version);
  free(msg->body);
  free(msg);
}
