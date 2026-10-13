#ifndef NETWORK_H
#define NETWORK_H

#include "includes.h"

#define TIMEOUT 2 // Timeout in seconds for UDP and TCP operations

int open_udp_socket(const char* dsip, const char* dsport, int* sockfd);

int send_receive_udp(const char* dsip, const char* dsport, const char* message, char* response, size_t response_size);

int send_receive_tcp(const char* dsip, const char* dsport, const char* message, char* response, size_t response_size);

int download_file(const char* dsip, const char* dsport, const char* filename, long* file_size);

#endif // NETWORK_H