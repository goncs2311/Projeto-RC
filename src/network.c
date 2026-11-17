#include "includes.h"

static int read_token(int fd, char *buf, size_t size) {
    size_t total = 0;
    char c;
    ssize_t n_read;

    if (size < 2) {
        return -1;
    }

    while (1) {

        n_read = read(fd, &c, 1);

        if (n_read < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("read");
            return -1;
        }

        if (n_read == 0) {
            return -1;
        }

        // Check if the end of the token was reached
        if (c == ' ' || c == '\n') {

            if (total == 0)
                return -1;

            buf[total] = '\0';

            return c;
        }

        // Check for invalid character or buffer overflow
        if (c == '\0' || total >= size - 1) {
            return -1;
        }

        buf[total++] = c;
    }
}

int write_all(int fd, const char *buf, size_t size) {
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

int read_line(int fd, char *buf, size_t size) {
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
            if (total > 0) {
                buf[total] = '\0';
                return (int)total;
            }

            errno = ECONNRESET;
            return -1;
        }

        buf[total++] = c;

        if (c == '\n') {
            buf[total] = '\0';
            return (int)total;   // Return number of bytes read
        }
    }

    buf[total] = '\0';
    errno = EMSGSIZE;
    return -1; // Message too long
}

// Function to set socket timeout for both send and receive operations
int set_socket_timeout(int fd) {
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
    struct addrinfo hints, *res;
    char msg[512];
    int separator;
    char response[4];
    char status[4];
    char size_str[9];
    long received_size;
    FILE *file;
    char buffer[4096];
    long remaining;
    ssize_t bytes_read;

    // Create TCP socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    // Set socket timeout
    if (set_socket_timeout(fd) < 0) {
        close(fd);
        return -1;
    }

    // Configure address information
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(peer_ip, peer_port, &hints, &res);
    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(errcode));
        close(fd);
        return -1;
    }

    // Connect to the peer
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        freeaddrinfo(res);
        close(fd);
        return -1;
    }

    freeaddrinfo(res);

    // Send GET request
    snprintf(msg, sizeof(msg), "GET %s\n", filename);

    if (write_all(fd, msg, strlen(msg)) == -1) {
        fprintf(stderr, "Error sending GET request.\n");
        close(fd);
        return -1;
    }

    // Read protocol command (RGT)
    separator = read_token(fd, response, sizeof(response));

    if (separator != ' ' || strcmp(response, "RGT") != 0) {
        fprintf(stderr, "Invalid response from peer.\n");
        close(fd);
        return -1;
    }

    // Read response status
    separator = read_token(fd, status, sizeof(status));

    if (separator == '\n' && strcmp(status, "NOK") == 0) {
        printf("Resource not available: %s.\n", filename);
        close(fd);
        return -1;
    }

    if (separator == '\n' && strcmp(status, "ERR") == 0) {
        printf("Syntax error in get.\n");
        close(fd);
        return -1;
    }

    if (separator != ' ' || strcmp(status, "OK") != 0) {
        fprintf(stderr, "Invalid response status.\n");
        close(fd);
        return -1;
    }

    // Read file size
    separator = read_token(fd, size_str, sizeof(size_str));

    if (separator != ' ') {
        fprintf(stderr, "Invalid file size field.\n");
        close(fd);
        return -1;
    }

    // Validate file size format 
    for (int i = 0; size_str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)size_str[i])) {
            fprintf(stderr, "Invalid file size format.\n");
            close(fd);
            return -1;
        }
    }
    
    // Convert file size to long 
    received_size = strtol(size_str, NULL, 10);

    // Validate file size range 
    if (received_size < 0 || received_size > 10000000) {
        fprintf(stderr, "Invalid file size received.\n");
        close(fd);
        return -1;
    }

    // Create temporary filename to avoid overwriting existing files
    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename);

    // Open temporary file for writing
    file = fopen(temp_filename, "wb");
    if (file == NULL) {
        perror("fopen");
        close(fd);
        return -1;
    }

    // Receive exactly the advertised number of bytes
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
            remove(temp_filename);
            close(fd);
            return -1;
        }

        if (bytes_read == 0) {
            fprintf(stderr, "Connection closed before file was fully received.\n");
            fclose(file);
            remove(temp_filename);
            close(fd);
            return -1;
        }

        if (fwrite(buffer, 1, (size_t)bytes_read, file) != (size_t)bytes_read) {
            perror("fwrite");
            fclose(file);
            remove(temp_filename);
            close(fd);
            return -1;
        }

        remaining -= bytes_read;
    }

    // validate final newline
    char end;
    ssize_t n;

    do {
        n = read(fd, &end, 1);
    } while (n < 0 && errno == EINTR);

    if (n != 1 || end != '\n') {
        fprintf(stderr, "Expected final newline after file data.\n");
        fclose(file);
        remove(temp_filename);
        close(fd);
        return -1;
    }

    // Close the temporary file
    if (fclose(file) != 0) {
        perror("fclose");
        remove(temp_filename);
        close(fd);
        return -1;
    }

    // Rename the temporary file to the final filename
    if (rename(temp_filename, filename) != 0) {
        perror("rename");
        remove(temp_filename);
        close(fd);
        return -1;
    }

    close(fd);
    *file_size = received_size;
    return 0;
}

int open_udp_socket(void) {
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
int send_receive_udp(int *fd, const char* dsip, const char* dsport, const char* message, char* response, size_t response_size) {
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
    n = sendto(*fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n==-1) {
        perror("sendto");
        freeaddrinfo(res);
        printf("erro a enviar a mensagem.\n");
        return -1;
    }

    // receiving the response from the server
    addrlen=sizeof(addr);
    n = recvfrom(*fd, response, response_size - 1, 0, (struct sockaddr*)&addr, &addrlen);
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
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Timeout receiving response.\n");
        } else if (errno == ECONNRESET) {
            printf("Connection closed before receiving a response.\n");
        } else if (errno == EMSGSIZE) {
            printf("Response is too long.\n");
        } else {
            perror("read");
        }
        return -1; 
    }

    response[n] = '\0';

    freeaddrinfo(res);
    close(fd);

    return 0;
}

int open_tcp_server(const char* tcpport) {
    int fd, errcode, n;
    struct addrinfo hints, *res;

    // Create TCP socket
    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1) {
        perror("socket");
        return -1;
    }

    // Allow reuse of the address
    int reuse = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        close(fd);
        return -1;
    }

    // Configure address information
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Get local address
    errcode = getaddrinfo(NULL, tcpport, &hints, &res);

    if (errcode != 0) {
        fprintf(stderr, "getaddrinfo: %s\n",
                gai_strerror(errcode));
        close(fd);
        return -1;
    }

    // Bind socket to the selected port
    n = bind(fd, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);

    if (n == -1) {
        perror("bind");
        close(fd);
        return -1;
    }

    // Start listening for incoming connections
    if (listen(fd, 5) == -1) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

int send_file_to_peer(int new_peer_fd, const char *filename) {
    FILE *file;
    char header[64];
    char buffer[4096];
    long fsize;
    long remaining;
    size_t bytes_read;

    // Open the file in binary mode
    file = fopen(filename, "rb");

    if (file == NULL) {
        write_all(new_peer_fd, "RGT NOK\n", 8);
        return -1;
    }

    // Determine file size
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        write_all(new_peer_fd, "RGT NOK\n", 8);
        return -1;
    }

    fsize = ftell(file);

    // Validate file size and return to the beginning
    if (fsize < 0 || fsize > 10000000 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        write_all(new_peer_fd, "RGT NOK\n", 8);
        return -1;
    }

    // Send response header
    snprintf(header, sizeof(header),
             "RGT OK %ld ", fsize);

    if (write_all(new_peer_fd, header, strlen(header)) < 0) {
        fclose(file);
        return -1;
    }

    // Send file data
    remaining = fsize;

    while (remaining > 0) {

        size_t to_read = sizeof(buffer);

        if (remaining < (long)to_read)
            to_read = (size_t)remaining;

        bytes_read = fread(buffer, 1, to_read, file);

        if (bytes_read == 0) {
            fclose(file);
            return -1;
        }

        if (write_all(new_peer_fd, buffer, bytes_read) < 0) {
            fclose(file);
            return -1;
        }

        remaining -= bytes_read;
    }

    fclose(file);

    // Send final newline
    if (write_all(new_peer_fd, "\n", 1) < 0)
        return -1;

    return 0;
}



