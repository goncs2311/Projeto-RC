#ifndef NETWORK_H
#define NETWORK_H

#include "includes.h"

#define TIMEOUT 2 // Timeout in seconds for UDP and TCP operations

int write_all(int fd, const char *buf, size_t count);

int read_line(int fd, char *buf, size_t size);

int set_socket_timeout(int fd);

int open_udp_socket(void);

int send_receive_udp(int *fd, const char* dsip, const char* dsport, const char* message, char* response, size_t response_size);

int send_receive_tcp(const char* dsip, const char* dsport, const char* message, char* response, size_t response_size);

int download_file(const char* dsip, const char* dsport, const char* filename, long* file_size);

int open_tcp_server(const char* tcpport);

int send_file_to_peer(int new_peer_fd, const char* filename);

#endif // NETWORK_H