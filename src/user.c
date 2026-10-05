#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "network.h"
#include "validation.h"
#include "handlers.h"

// IP do lab: 192.168.1.1
// IP fora do lab: tejo.tecnico.ulisboa.pt
#define DSIP "193.136.138.142"
#define DSPORT "59000"

int main(int argc, char *argv[]) {
    char dsip[30] = DSIP;
    char dsport[6] = DSPORT;
    int tcpport = 0;
    int opt;

    // Leitura dos argumentos: ./user -m peerport [-n DSIP] [-p DSport]
    while ((opt = getopt(argc, argv, "m:n:p:")) != -1) {
        switch (opt) {
            case 'm':
                tcpport = atoi(optarg);
                break;
            case 'n':
                strncpy(dsip, optarg, sizeof(dsip) - 1);
                break;
            case 'p':
                strncpy(dsport, optarg, sizeof(dsport) - 1);
                break;
            default:
                fprintf(stderr, "Uso: %s -m peerport [-n DSIP] [-p DSport]\n", argv[0]);
                exit(1);
        }
    }

    if (tcpport <= 0) {
        fprintf(stderr, "Erro: O argumento -m <peerport> e obrigatorio.\n");
        exit(1);
    }

    char input[128];
    char command[20]; 
    char uid[10];
    char password[20];
    char filename[128];
    char label[21];
    int session_state = LOGGED_OUT;

    while (1) {
        printf("> ");
        fflush(stdout); 
        
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        
        if (sscanf(input, "%19s", command) != 1) continue;

        if (strcmp(command, "login") == 0) {
            handle_login(input, uid, password, tcpport, dsip, dsport, &session_state);
        } else if (strcmp(command, "logout") == 0) {
            handle_logout(uid, password, dsip, dsport, &session_state);
        } else if (strcmp(command, "unregister") == 0) {
            handle_unregister(uid, password, dsip, dsport, &session_state);
        } else if (strcmp(command, "exit") == 0) {
            if (session_state == LOGGED_OUT) {
                break;
            } else {
                printf("Please logout first.\n");
            }
        } else  if (strcmp(command, "publish") == 0) {
            handle_publish(input, uid, password, filename, label, dsip, dsport, &session_state);
        } else if (strcmp(command, "remove") == 0) {
            handle_remove(input, uid, password, filename, dsip, dsport, &session_state);
        } else if (strcmp(command, "list") == 0) {
            handle_list(dsip, dsport);
        } else if (strcmp(command, "versions") == 0) {
            handle_versions(input, filename, dsip, dsport);
        } else {
            printf("Unknown command. Available commands: login, logout, unregister, exit, publish, remove, list, versions\n");
        }
    }

    return 0;
}