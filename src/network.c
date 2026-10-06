#include "includes.h"

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

static int connect_with_timeout(int fd, const struct sockaddr *addr,
                                socklen_t addrlen) {

    int flags;
    int result;
    int error;
    socklen_t error_size = sizeof(error);

    struct timeval timeout;
    fd_set writefds;

    // Get current socket flags
    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    // Set socket to non-blocking
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        return -1;
    }

    // Start connection
    result = connect(fd, addr, addrlen);

    if (result == 0) {
        // Connection established immediately
        fcntl(fd, F_SETFL, flags);
        return 0;
    }

    if (errno != EINPROGRESS) {
        perror("connect");
        fcntl(fd, F_SETFL, flags);
        return -1;
    }

    // Wait for the socket to become writable
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    result = select(fd + 1, NULL, &writefds, NULL, &timeout);

    if (result == 0) {
        printf("Timeout: connection to server took too long.\n");
        fcntl(fd, F_SETFL, flags);
        return -1;
    }

    if (result == -1) {
        if (errno == EINTR) {
            printf("select interrupted.\n");
        } else {
            perror("select");
        }

        fcntl(fd, F_SETFL, flags);
        return -1;
    }

    // Check if the connection actually succeeded
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &error_size) == -1) {
        perror("getsockopt");
        fcntl(fd, F_SETFL, flags);
        return -1;
    }

    if (error != 0) {
        errno = error;
        perror("connect");
        fcntl(fd, F_SETFL, flags);
        return -1;
    }

    // Restore blocking mode
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl");
        return -1;
    }

    return 0;
}

// Function to send a message to the server and receive a response using UDP

int send_receive_udp(const char* dsip, const char* dsport, const char* message, char* response, size_t response_size) {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;    // hints are the address info from the user, res is the address info from the server
    struct sockaddr_in addr;    // address from whoever sent the message (server or client)

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

    // setting up the hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;

    //getting the server address
    errcode=getaddrinfo(dsip, dsport, &hints, &res);
    if (errcode!=0) {
        close(fd);
        printf("erro a obter o endereço do servidor.\n");
        return -1; 
    }

    // sending the message to the server
    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n==-1) {
        perror("sendto");
        freeaddrinfo(res);
        close(fd);
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
        close(fd);
        return -1;
    }

    response[n] = '\0';

    freeaddrinfo(res);
    close(fd);

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

   if (connect_with_timeout(fd, res->ai_addr, res->ai_addrlen) == -1) {
        freeaddrinfo(res);
        close(fd);
        printf("Error connecting to server.\n");
        return -1;
    }

    n = write(fd, message, strlen(message));
    if (n == -1) {
        perror("write");
        freeaddrinfo(res);
        close(fd);
        printf("Error sending message.\n");
        return -1; 
    }

    n = read(fd, response, response_size - 1);
    if (n == -1) {
        perror("read");
        freeaddrinfo(res);
        close(fd);
        printf("Error receiving response.\n");
        return -1; 
    }

    response[n] = '\0';

    // é necessário???
    //write(1, response, n); // write the response to stdout

    freeaddrinfo(res);
    close(fd);

    return 0;
}