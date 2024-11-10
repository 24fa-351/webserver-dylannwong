#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "http_message.h"

int calc_endpoint(int client_socket, char *path) {
  // if CALC endpoint
  int a, b;
  if (sscanf(path, "/calc/%d/%d", &a, &b) == 2) {
    char response[1024];
    int response_length =
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %d\r\n"
                 "Content-Type: text/html\r\n"
                 "\r\n"
                 "<html><body><h1>Sum: %d</h1></body></html>\n",
                 39 + snprintf(NULL, 0, "%d", a + b), a + b);

    if (write(client_socket, response, response_length) != response_length) {
      perror("write failed");
      return -1;
    }

    return response_length;
  } else {
    char err_response[1024];
    int response_length =
        snprintf(err_response, sizeof(err_response),
                 "<html><body><h1>404 Not Found</h1></body></html>\n");

    write(client_socket, err_response, response_length);
    return 0;
  }
}

// if STATS endpoint
int stats_endpoint(int client_socket, char *path, int request_count,
                   int sent_bytes, int received_bytes) {

  char body[1024];

  int body_length = snprintf(body, sizeof(body),
                             "<html><body><h1>Stats</h1>"
                             "<p>Request count: %d</p>"
                             "<p>Sent bytes: %d</p>"
                             "<p>Received bytes: %d</p></body></html>\n",
                             request_count, sent_bytes, received_bytes);

  char response[1024];
  int response_length = snprintf(response, sizeof(response),
                                 "HTTP/1.1 200 OK\r\n"
                                 "Content-Length: %d\r\n"
                                 "Content-Type: text/html\r\n"
                                 "\r\n"
                                 "%s",
                                 body_length, body);

  if (write(client_socket, response, response_length) != response_length) {
    perror("write failed");
    return -1;
  }
  return response_length;
}
// if IMAGES endpoint
int images_endpoint(int client_socket, char *path) {
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
                   "Content-Type: image/jpeg\r\n"
                   "Content-Disposition: inline\r\n"
                   "\r\n",
                   (long)file_stat.st_size);
      write(client_socket, response_header, response_header_length);
      write(client_socket, file_content, file_stat.st_size);

      free(file_content);
      return response_header_length + file_stat.st_size;
      ;
    }
  } else {
    char err_response[1024];
    int response_length =
        snprintf(err_response, sizeof(err_response),
                 "<html><body><h1>404 Not Found</h1></body></html>\n");

    write(client_socket, err_response, response_length);
    return 0;
  }
  return 0;
}
// if RANDOM endpoint
int random_endpoint(int client_socket, char *path) {
  char response[1024];
  srand(time(NULL));
  int a = rand() % 100;
  int response_length =
      snprintf(response, sizeof(response),
               "HTTP/1.1 200 OK\r\n"
               "Content-Length: %d\r\n"
               "Content-Type: text/html\r\n"
               "\r\n"
               "<html><body><h1>Random Number: %d</h1></body></html>\n",
               49 + snprintf(NULL, 0, "%d", a), a);

  write(client_socket, response, response_length);

  return response_length;
}