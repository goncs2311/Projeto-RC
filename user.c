#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

#define DSIP "193.136.138.142"
#define DSPORT "59000"

int send_recieve_udp(const char* dsip, char* dsport, const char* message, char* response, size_t response_size) {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;    // hints are the address info from the user, res is the address info from the server
    struct sockaddr_in addr;    // address from whoever sent the message (server or client)

    //starting UDP socket
    fd= socket(AF_INET, SOCK_DGRAM, 0);
    if (fd==-1) return -1; /*error*/

    // setting up the hints for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;

    //getting the server address
    errcode=getaddrinfo(dsip, dsport, &hints, &res);
    if (errcode!=0) {
        close(fd);
        return -1; 
        /*error*/
    }

    // sending the message to the server
    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if (n==-1) {
        freeaddrinfo(res);
        close(fd);
        return -1;
        /*error*/
    }

    // receiving the response from the server
    addrlen=sizeof(addr);
    n = recvfrom(fd, response, response_size - 1, 0, (struct sockaddr*)&addr, &addrlen);
    if (n==-1) {
        freeaddrinfo(res);
        close(fd);
        return -1; 
        /*error*/
    }

    response[n] = '\0'; // Null-terminate the response

    freeaddrinfo(res);
    close(fd);

    return 0;
}

int main(int argc, char *argv[]) {
    char dsip[16] = DSIP;
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
        if (sscanf(input, "%2s", command) != 1) {
            continue; // Se o utilizador apenas carregou no Enter, ignora e pede de novo
        }

        // Compara o comando lido com os comandos válidos
        if (strcmp(command, "LIN") == 0) {
            // O login precisa do UID (6 dígitos) e da password (8 caracteres)
            if (sscanf(input, "%*s %9s %19s", uid_str, password) == 2) {
                char msg[128];
                char response[128];

                printf("A preparar para fazer login do utilizador: %s\n", uid_str);
                snprintf(msg, sizeof(msg), "LIN %s %s %d", uid_str, password, tcpport);
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
