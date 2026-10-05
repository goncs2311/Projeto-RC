#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>
#include <sys/select.h>
#include <sys/socket.h>

#define TIMEOUT 5 // Timeout in seconds for UDP and TCP operations

int send_receive_udp(const char* dsip, const char* dsport, const char* message, char* response, size_t response_size);

int send_receive_tcp(const char* dsip, const char* dsport, const char* message, char* response, size_t response_size);

#endif // NETWORK_H