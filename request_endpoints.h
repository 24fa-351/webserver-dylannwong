#ifndef REQUEST_ENDPOINTS_H
#define REQUEST_ENDPOINTS_H

int calc_endpoint(int client_socket, char *path);

int stats_endpoint(int client_socket, char *path, int request_count,
                   int sent_bytes, int received_bytes);

int images_endpoint(int client_socket, char *path);

int random_endpoint(int client_socket, char *path);

#endif
