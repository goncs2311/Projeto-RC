#include "includes.h"

static int write_all(int fd, const char *buf, size_t size) {
    size_t total = 0;
    ssize_t bytes_written;

    while (total < size) {
        bytes_written = write(fd, buf + total, size - total);

        if (bytes_written < 0) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, retry
            }
            perror("write");
            return -1; // Error occurred
        }
        total += bytes_written;

        if (bytes_written == 0) {
            return -1; // Connection closed unexpectedly
        }
    }

    return 0; // Success
}

static int read_line(int fd, char *buf, size_t size) {
    size_t total = 0;
    char c;
    ssize_t n_read;

    if (size < 2) {
        return -1;
    }

    while (total < size - 1) {
        n_read = read(fd, &c, 1);

        if (n_read < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted by signal, retry
            }
            perror("read");
            return -1;  // Error occurred
        }

        if (n_read == 0) {
            break;  // Connection closed
        }

        buf[total++] = c;

        if (c == '\n') {
            buf[total] = '\0';
            return (int)total;   // Return number of bytes read
        }
    }

    buf[total] = '\0';
    return -1; // Message too long or connection closed before newline
}

// Function to set socket timeout for both send and receive operations
static int set_socket_timeout(int fd) {
    struct timeval timeout;

    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt SO_RCVTIMEO");
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
                   &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt SO_SNDTIMEO");
        return -1;
    }

    return 0;
}


int download_file(const char *peer_ip, const char *peer_port, const char *filename, long *file_size) {
    int fd, errcode;
    int n;
    struct addrinfo hints, *res;
    char msg[512];
    char response[512];
    char *size_str;
    long received_size;
    FILE *file;
    char buffer[4096];
    long remaining;
    ssize_t bytes_read;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    if (set_socket_timeout(fd) < 0) {
        close(fd);
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(peer_ip, peer_port, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        close(fd);
        return -1;
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    freeaddrinfo(res);

    /* Send GET request */
    snprintf(msg, sizeof(msg), "GET %s\n", filename);

    if (write_all(fd, msg, strlen(msg)) == -1) {
        fprintf(stderr, "Error sending GET request.\n");
        close(fd);
        return -1;
    }

    /* Receive the response line */
    n = read_line(fd, response, sizeof(response));
    if (n == -1) {
        fprintf(stderr, "Error receiving response from peer.\n");
        close(fd);
        return -1;
    }

    if (strncmp(response, "RGT NOK", 7) == 0) {
        printf("Resource not available: %s.\n", filename);
        close(fd);
        return -1;
    }

    if (strncmp(response, "RGT ERR", 7) == 0) {
        printf("Syntax error in get.\n");
        close(fd);
        return -1;
    }

    if (strncmp(response, "RGT OK ", 7) != 0) {
        fprintf(stderr, "Invalid response from peer.\n");
        close(fd);
        return -1;
    }

    /* Extract the advertised file size */
    size_str = response + 7;
    received_size = strtol(size_str, NULL, 10);

    if (received_size < 0 || received_size > 10000000) {
        fprintf(stderr, "Invalid file size received.\n");
        close(fd);
        return -1;
    }

    /* Create the destination file */
    file = fopen(filename, "wb");
    if (file == NULL) {
        perror("fopen");
        close(fd);
        return -1;
    }

    /* Receive exactly the advertised number of bytes */
    remaining = received_size;

    //read all TODO
    while (remaining > 0) {
        size_t to_read = sizeof(buffer);
        if (remaining < (long)to_read) {
            to_read = (size_t)remaining;
        }

        bytes_read = read(fd, buffer, to_read);

        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("read");
            fclose(file);
            remove(filename);
            close(fd);
            return -1;
        }

        if (bytes_read == 0) {
            fprintf(stderr, "Connection closed before file was fully received.\n");
            fclose(file);
            remove(filename);
            close(fd);
            return -1;
        }

        if (fwrite(buffer, 1, (size_t)bytes_read, file) !=
            (size_t)bytes_read) {
            perror("fwrite");
            fclose(file);
            remove(filename);
            close(fd);
            return -1;
        }

        remaining -= bytes_read;
    }

    if (fclose(file) != 0) {
        perror("fclose");
        remove(filename);
        close(fd);
        return -1;
    }

    close(fd);

    *file_size = received_size;
    return 0;
}

int open_udp_socket(const char* dsip, const char* dsport, int* sockfd) {
    int fd;

    //starting UDP socket
    fd= socket(AF_INET, SOCK_DGRAM, 0);
    if (fd==-1) {
        printf("Error creating socket\n");
        return -1;
    }

    if (set_socket_timeout(fd) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

// Function to send a message to the server and receive a response using UDP
int send_receive_udp(int fd, const char* dsip, const char* dsport, const char* message, char* response, size_t response_size) {
    int errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;    // hints are the address info from the user, res is the address info from the server
    struct sockaddr_in addr;    // address from whoever sent the message (server or client)

    // setting up the hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;

    //getting the server address
    errcode=getaddrinfo(dsip, dsport, &hints, &res);
    if (errcode!=0) {
        printf("erro a obter o endereço do servidor.\n");
        return -1; 
    }

    // sending the message to the server
    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n==-1) {
        perror("sendto");
        freeaddrinfo(res);
        printf("erro a enviar a mensagem.\n");
        return -1;
    }

    // receiving the response from the server
    addrlen=sizeof(addr);
    n = recvfrom(fd, response, response_size - 1, 0, (struct sockaddr*)&addr, &addrlen);
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Timeout: servidor nao respondeu.\n");
        } else {
            perror("recvfrom");
        }

        freeaddrinfo(res);
        return -1;
    }

    response[n] = '\0';

    freeaddrinfo(res);

    return 0;
}

// Function to send a message to the server and receive a response using TCP
int send_receive_tcp(const char* dsip, const char* dsport, const char* message, char* response, size_t response_size) {
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;    // hints are the address info from the user, res is the address info from the server

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("Error creating socket\n");
        return -1;
    }

    if (set_socket_timeout(fd) < 0) {
        close(fd);
        return -1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(dsip, dsport, &hints, &res);
    if (errcode != 0) {
        close(fd);
        printf("Error getting server address.\n");
        return -1; 
    }

    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    n = write_all(fd, message, strlen(message));
    if (n == -1) {
        freeaddrinfo(res);
        close(fd);
        printf("Error sending message.\n");
        return -1; 
    }

    n = read_line(fd, response, response_size);
    if (n == -1) {
        freeaddrinfo(res);
        close(fd);
        printf("Error receiving response.\n");
        return -1; 
    }

    response[n] = '\0';

    freeaddrinfo(res);
    close(fd);

    return 0;
}

// send_recieve_tcp_peer