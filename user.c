#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

#define DSIP "tejo.tecnico.ulisboa.pt"
#define DSPORT "59000"

#define LOGGED_OUT 0
#define LOGGED_IN 1

int send_recieve_udp(const char* dsip, char* dsport, const char* message, char* response, size_t response_size) {
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
    if (n==-1) {
        freeaddrinfo(res);
        close(fd);
        printf("erro a receber a resposta.\n");
        return -1; 
    }

    response[n] = '\0';

    freeaddrinfo(res);
    close(fd);

    return 0;
}

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
    int session_state = LOGGED_OUT;

    while (1) {
        printf("> ");
        fflush(stdout); 
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        if (sscanf(input, "%19s", command) != 1) {
            continue;
        }

        if (strcmp(command, "login") == 0) {
            if (sscanf(input, "%*s %9s %19s", uid, password) == 2) {
                char msg[128];
                char response[128];

                // Protocolo: LIN UID password peerTCPport\n
                snprintf(msg, sizeof(msg), "LIN %s %s %d\n", uid, password, tcpport);
                
                if (send_recieve_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
                    if (strncmp(response, "RLI OK", 6) == 0) {
                        session_state = LOGGED_IN;
                        printf("successeful login.\n");
                    } else if (strncmp(response, "RLI NOK", 7) == 0) {
                        printf("incorrect login attempt.\n");
                    } else if (strncmp(response, "RLI REG", 7) == 0) {
                        session_state = LOGGED_IN;
                        printf("new user registered.\n");
                    }
                    else if (strncmp(response, "RLI ERR", 7) == 0) {
                        printf("sintax error in login.\n");
                    }
                }
            }
            
        } else if (strcmp(command, "logout") == 0) {
            char msg[128];
            char response[128];

            //TODO: VERIFICAR SE ESTA CERTO 
            if (session_state == LOGGED_OUT) {
                printf("user not logged in.\n");
                continue;
            }
            
            // Protocolo: LOU UID password
            snprintf(msg, sizeof(msg), "LOU %s %s\n", uid, password);
            //TODO: receber uid/password do terminal???            
            
            if (send_recieve_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
                if (strncmp(response, "RLO OK", 6) == 0) {
                    session_state = LOGGED_OUT;
                    printf("successful logout.\n");
                } else if (strncmp(response, "RLO NLG", 7) == 0) {
                    printf("user not logged in.\n");
                } else if (strncmp(response, "RLO WRP", 7) == 0) {
                    printf("incorrect logout attempt.\n");
                    printf("incorrect password.\n");
                } else if (strncmp(response, "RLO ERR", 7) == 0) {
                    printf("sintax error in logout.\n");
                }
            }
            
        } else if (strcmp(command, "unregister") == 0) {
            char msg[128];
            char response[128];

            snprintf(msg, sizeof(msg), "UNR %s %s\n", uid, password);

            if (send_recieve_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
                if (strncmp(response, "RUR OK", 6) == 0) {
                    session_state = LOGGED_OUT;
                    printf("successful unregister.\n");
                } else if (strncmp(response, "RUR NOK", 7) == 0) {
                    printf("user not logged in.\n");
                } else if (strncmp(response, "RUR UNR", 7) == 0) {
                    printf("unknown user.\n");
                } else if (strncmp(response, "RUR WRP", 7) == 0) {
                    printf("incorrect unregister attempt.\n");
                } else if (strncmp(response, "RUR ERR", 7) == 0) {
                    printf("sintax error in unregister.\n");
                }
            }
            
        } else if (strcmp(command, "exit") == 0) {
            if (session_state == LOGGED_OUT) {
                // printf("exiting...\n");
                break;
            } else {
                printf("Please logout first.\n");
            }
            
        } else {
            printf("Unknown command. Available commands: login, logout, unregister, exit\n");
        }
    }

    return 0;
}