#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

#define PORT "58001"    // the port (from the server) that users will be connecting to

void readCommand(char* input, char* command, int *uid, int *tcpport, char* password) {
    sscanf(input, "%s %d %d %s", command, uid, tcpport, password);
}

int udp() {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;    // hints are the address info from the user, res is the address info from the server
    struct sockaddr_in addr;    // address from whoever sent the message (server or client)
    char buffer[128];

    fd= socket(AF_INET, SOCK_DGRAM, 0);
    if (fd==-1) /*error*/exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;

    errcode=getaddrinfo("tejo.tecnico.ulisboa.pt", PORT, &hints, &res);
    if (errcode!=0) /*error*/ exit(1);

    n=sendto(fd, "Hello gostoso!\n", 16, 0, res->ai_addr, res->ai_addrlen);
    if (n==-1) /*error*/ exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
    if (n==-1) /*error*/ exit(1);

    write(1, "echo: ", 6); write(1, buffer, n);

    freeaddrinfo(res);
    close(fd);

    return 0;
}

int main(int argc, char *argv[]) {
    char input[128];
    char command[3];
    char dsip[16];
    int uid, tcpport, dsport;
    char password[9];

    tcpport = atoi(argv[2]);
    if (argc > 3) {
        strcpy(dsip, argv[4]);
    }
    if (argc > 5) {
        dsport = atoi(argv[6]);
    }

    printf("tcpport: %d, dsip: %s, dsport: %d\n", tcpport, dsip, dsport);

    fgets(input, 128, stdin);
    // ler só as primeiras 3 letras (comando)
    strncpy(command, input, 3);
    command[3] = '\0';
    // depois guardar o resto em variaveis
    readCommand(input, command, &uid, &tcpport, password);

    
    if (strcmp(command, "LIN", 0) == 0) {
        // login(uid, tcpport, password);
    } else if (strcmp(command, "RLI", 0) == 0) {
        // register(uid, tcpport, password);
    } else {
        printf("Invalid command\n");
    }

    printf("command: %s", input);

    return 0;
}