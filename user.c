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

    printf("A enviar a mensagem: %s\n", message);

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

    response[n] = '\0'; // Null-terminate the response

    freeaddrinfo(res);
    close(fd);

    return 0;
}

int main(int argc, char *argv[]) {
    char dsip[30] = DSIP;
    char dsport[6] = DSPORT;
    int tcpport = 0;
    int opt;

    // Leitura correta dos argumentos com getopt: ./user -m peerport [-n DSIP] [-p DSport][cite: 1]
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
    char command[20]; // Aumentado para evitar estouro de memória!
    char uid_str[10];
    char password[20];

    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        if (sscanf(input, "%19s", command) != 1) {
            continue;
        }

        // O utilizador digita "login UID password" no terminal[cite: 1]
        if (strcmp(command, "login") == 0) {
            if (sscanf(input, "%*s %9s %19s", uid_str, password) == 2) {
                char msg[128];
                char response[128];

                printf("A preparar para fazer login do utilizador: %s\n", uid_str);
                // Protocolo: LIN UID password peerTCPport\n[cite: 1]
                snprintf(msg, sizeof(msg), "LIN %s %s %d\n", uid_str, password, tcpport);
                
                if (send_recieve_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
                    if (strncmp(response, "RLI OK", 6) == 0) {
                        printf("Login efetuado com sucesso.\n");
                    } else if (strncmp(response, "RLI NOK", 7) == 0) {
                        printf("Tentativa de login incorreta.\n");
                    } else if (strncmp(response, "RLI REG", 7) == 0) {
                        printf("Novo utilizador registado.\n");
                    } else {
                        printf("Resposta do servidor: %s", response);
                    }
                }

            } else {
                printf("Erro de sintaxe. Uso correto: login UID password\n");
            }
            
        } else if (strcmp(command, "logout") == 0) {
            printf("A preparar logout...\n");
            // TODO: Implementar envio de 'LOU UID password\n'[cite: 1]
            
        } else if (strcmp(command, "unregister") == 0) {
            printf("A preparar unregister...\n");
            // TODO: Implementar envio de 'UNR UID password\n'[cite: 1]
            
        } else if (strcmp(command, "exit") == 0) {
            printf("A sair da aplicacao...\n");
            break;
            
        } else {
            printf("Comando desconhecido. Comandos disponiveis: login, logout, unregister, exit\n");
        }
    }

    return 0;
}

/*int main(int argc, char *argv[]) {
    char dsip[30] = DSIP;
    char dsport[6] = DSPORT;
    int tcpport;

    tcpport = atoi(argv[2]);
    if (argc > 3) {
        strcpy(dsip, argv[4]); // verify if valid
    }
    if (argc > 5) {
        strcpy(dsport, argv[6]); // verify if valid
    }

    // Ciclo infinito para ler o input do utilizador
    char input[128];
    char command[3];
    char uid_str[10];
    char password[20];

    while (1) {
        printf("> "); // Prompt visual para o utilizador
        
        // Lê a linha inteira do stdin
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // Sai do ciclo se houver EOF (Ctrl+D)
        }

        // Lê a primeira palavra para a variável 'command'
        if (sscanf(input, "%3s", command) != 1) {
            continue; // Se o utilizador apenas carregou no Enter, ignora e pede de novo
        }

        // Compara o comando lido com os comandos válidos
        if (strcmp(command, "LIN") == 0) {
            // O login precisa do UID (6 dígitos) e da password (8 caracteres)
            if (sscanf(input, "%*s %6s %8s", uid_str, password) == 2) {
                char msg[128];
                char response[128];

                printf("A preparar para fazer login do utilizador: %s\n", uid_str);
                snprintf(msg, sizeof(msg), "LIN %s %s %d\n", uid_str, password, tcpport);
                send_recieve_udp(dsip, dsport, msg, response, sizeof(response));
                
                if (strncmp(response, "RLI OK", 6) == 0) {
                    printf("successeful login.\n");
                } else if (strncmp(response, "RLI NOK", 7) == 0) {
                    printf("incorrect login attempt.\n");
                } else if (strncmp(response, "RLI REG", 7) == 0) {
                    printf("new user registered.\n");
                }

            } else {
                printf("Erro de sintaxe. Uso correto: LIN UID password\n");
            }
            
        } else if (strcmp(command, "RLI") == 0) {
            // O register precisa do UID (6 dígitos), do TCP port (5 dígitos) e da password (8 caracteres)
            if (sscanf(input, "%*s %9s %d %19s", uid_str, &tcpport, password) == 3) {
                printf("A preparar para fazer register do utilizador: %s\n", uid_str);
                char msg[128];
                char response[128];
                snprintf(msg, sizeof(msg), "RLI %s %d %s", uid_str, tcpport, password);
                send_recieve_udp(dsip, dsport, msg, response, sizeof(response));
                printf("Resposta do servidor: %s\n", response);
                // TODO: Implementar envio da mensagem UDP 'RLI UID peerTCPport password'[cite: 1]
            } else {
                printf("Erro de sintaxe. Uso correto: register UID TCPport password\n");
            }
        } else if (strcmp(command, "LOU") == 0) {
            printf("A preparar logout...\n");
            // TODO: Implementar envio da mensagem UDP 'LOU UID password'[cite: 1]
            
        } else if (strcmp(command, "UNR") == 0) {
            printf("A preparar unregister...\n");
            // TODO: Implementar envio da mensagem UDP 'UNR UID password'[cite: 1]
            
        } else if (strcmp(command, "exit") == 0) {
            // Se o utilizador ainda estiver logado, a aplicação deve avisar para fazer logout primeiro[cite: 1]
            // TODO: Adicionar verificação de estado de sessão
            printf("A sair da aplicacao...\n");
            break; // Quebra o ciclo e termina o programa
            
        } else {
            printf("Comando desconhecido. Comandos disponiveis: login, logout, unregister, exit\n");
        }
    }

    return 0;
}
*/