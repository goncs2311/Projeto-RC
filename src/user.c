#include "includes.h"

// IP do lab: 192.168.1.1
// IP fora do lab: tejo.tecnico.ulisboa.pt
#define DSIP "193.136.138.142"
#define DSPORT "59000"

int main(int argc, char *argv[]) {
    char dsip[30] = DSIP;
    char dsport[6] = DSPORT;
    char tcpport[6] = "";
    int opt;

    // Leitura dos argumentos: ./user -m peerport [-n DSIP] [-p DSport]
    while ((opt = getopt(argc, argv, "m:n:p:")) != -1) {
        switch (opt) {
            case 'm':
                strncpy(tcpport, optarg, sizeof(tcpport) - 1);
                tcpport[sizeof(tcpport) - 1] = '\0';
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

    if (strlen(tcpport) == 0) {
        fprintf(stderr, "Erro: O argumento -m <peerport> e obrigatorio.\n");
        exit(1);
    }

    char input[128];
    char command[20]; 
    char uid[10];
    char uid_peer[10] = "";
    char password[20];
    char filename[128];
    char label[21];
    int session_state = LOGGED_OUT;
    int fd;

    fd = open_udp_socket();

    if (fd < 0) {
        fprintf(stderr, "Error opening UDP socket\n");
        exit(1);
    }

    int tcp_fd = open_tcp_server(tcpport);
    fprintf(stderr, "TCP socket opened on port %s\n", tcpport);
    if (tcp_fd < 0) {
        fprintf(stderr, "Error opening TCP socket on port %s\n", tcpport);
        close(fd);
        exit(1);
    }

    fd_set readfds;
    
    int max_fd = tcp_fd > STDIN_FILENO ? tcp_fd : STDIN_FILENO;

    printf("> ");
    fflush(stdout);

    while (1) {

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(tcp_fd, &readfds);

        int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ready < 0) {
            if (errno == EINTR) continue; // Interrupted by signal, retry
            perror("select");
            break;
        }

        // verificar se existem comandos do user
        if (FD_ISSET(STDIN_FILENO, &readfds)) {

            if (fgets(input, sizeof(input), stdin) == NULL) break;

            if (sscanf(input, "%19s", command) != 1) {
                printf("> ");
                fflush(stdout);
                continue;
            }

            if (strcmp(command, "login") == 0) {
                handle_login(input, uid, password, tcpport, dsip, dsport, &session_state, &fd);
            } else if (strcmp(command, "logout") == 0) {
                handle_logout(uid, password, dsip, dsport, &session_state, &fd);
            } else if (strcmp(command, "unregister") == 0) {
                handle_unregister(uid, password, dsip, dsport, &session_state, &fd);
            } else if (strcmp(command, "exit") == 0) {
                if (session_state == LOGGED_OUT) {
                    break;
                } else {
                    printf("Please logout first.\n");
                }
            } else  if (strcmp(command, "publish") == 0) {
                handle_publish(input, uid, password, filename, label, dsip, dsport, &session_state, &fd);
            } else if (strcmp(command, "remove") == 0) {
                handle_remove(input, uid, password, filename, dsip, dsport, &session_state, &fd);
            } else if (strcmp(command, "list") == 0) {
                handle_list(dsip, dsport, &fd);
            } else if (strcmp(command, "versions") == 0) {
                handle_versions(input, filename, dsip, dsport, uid_peer);
            } else if (strcmp(command, "get") == 0) {
                handle_get(input, filename, uid_peer, dsip, dsport, &fd);
            } else {
                printf("Unknown command. Available commands: login, logout, unregister, exit, publish, remove, list, versions, get\n");
            }
        }

        // verificar se existem pedidos de outros peers
        if (FD_ISSET(tcp_fd, &readfds)) {
            int new_peer_fd = accept(tcp_fd, NULL, NULL);

            if (new_peer_fd < 0) {
                perror("accept");
                continue;
            } else {
                handle_peer_request(new_peer_fd, uid, dsip, dsport, &fd, &session_state);
                close(new_peer_fd);
            }
        }

        printf("> ");
        fflush(stdout);
    }

    close(fd);
    close(tcp_fd);

    return 0;
}