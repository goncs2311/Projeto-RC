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

// IP do lab: 192.168.1.1
// IP fora do lab: tejo.tecnico.ulisboa.pt
#define DSIP "193.136.138.142"
#define DSPORT "59000"

#define LOGGED_OUT 0
#define LOGGED_IN 1

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

int send_receive_tcp(const char* dsip, const char* dsport, const char* message, char* response, size_t response_size) {
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;    // hints are the address info from the user, res is the address info from the server

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("Error creating socket\n");
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

    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) {
        perror("connect");
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

int valid_filename(const char *filename) {
    int len = strlen(filename);

    // Maximum 24 characters
    if (len > 24) {
        return 0;
    }

    // Find the dot
    const char *dot = strrchr(filename, '.');
    if (dot == NULL) {
        return 0;
    }
    if (dot == filename) {
        return 0;
    }

    // Extension must have exactly 3 characters
    if (strlen(dot + 1) != 3) {
        return 0;
    }

    // Check filename base
    for (const char *p = filename; p < dot; p++) {
        if (!isalnum(*p) && *p != '-' && *p != '_') {
            return 0;
        }
    }

    // Check extension
    for (const char *p = dot + 1; *p != '\0'; p++) {
        if (!isalnum(*p)) {
            return 0;
        }
    }

    return 1;
}

int valid_label(const char *label) {
    int len = strlen(label);

    // Length must be between 1 and 20
    if (len < 1 || len > 20) {
        return 0;
    }

    // Only letters, digits, '-' and '_'
    for (int i = 0; i < len; i++) {
        if (!isalnum(label[i]) && label[i] != '-' && label[i] != '_') {
            return 0;
        }
    }

    return 1;
}

void handle_login(const char *input, char *uid, char *password, int tcpport, const char *dsip, const char *dsport, int *session_state) {
    char msg[128];
    char response[128];
    char extra[2];

    if (sscanf(input, "%*s %9s %19s %1s", uid, password, extra) != 2) {
        printf("Syntax error in login.\n");
        return;
    }

    if (*session_state == LOGGED_IN) {
        printf("User already logged in.\n");
        return;
    }

    snprintf(msg, sizeof(msg), "LIN %s %s %d\n", uid, password, tcpport);

    if (send_receive_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
        if (strncmp(response, "RLI OK", 6) == 0) {
            *session_state = LOGGED_IN;
            printf("Successful login.\n");
        } else if (strncmp(response, "RLI NOK", 7) == 0) {
            printf("Incorrect login attempt.\n");
        } else if (strncmp(response, "RLI REG", 7) == 0) {
            *session_state = LOGGED_IN;
            printf("New user registered.\n");
        } else if (strncmp(response, "RLI ERR", 7) == 0) {
            printf("Syntax error in login.\n");
        }
    }
}

void handle_logout(const char *uid, const char *password, const char *dsip, const char *dsport, int *session_state) {
    char msg[128];
    char response[128];

    if (*session_state == LOGGED_OUT) {
        printf("User not logged in.\n");
        return;
    }
    
    // Protocolo: LOU UID password
    snprintf(msg, sizeof(msg), "LOU %s %s\n", uid, password);         
    
    if (send_receive_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
        if (strncmp(response, "RLO OK", 6) == 0) {
            *session_state = LOGGED_OUT;
            printf("Successful logout.\n");
        } else if (strncmp(response, "RLO NLG", 7) == 0) {
            printf("User not logged in.\n");
        } else if (strncmp(response, "RLO WRP", 7) == 0) {
            printf("Incorrect password.\n");
        } else if (strncmp(response, "RLO UNR", 7) == 0) {
            printf("User not registered.\n");
        } else if (strncmp(response, "RLO ERR", 7) == 0) {
            printf("Sintax error in logout.\n");
        }
    }
}

void handle_unregister(const char *uid, const char *password, const char *dsip, const char *dsport, int *session_state) {
    char msg[128];
    char response[128];

    snprintf(msg, sizeof(msg), "UNR %s %s\n", uid, password);

    if (*session_state == LOGGED_OUT) {
        printf("User not logged in.\n");
        return;
    }

    if (send_receive_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
        if (strncmp(response, "RUR OK", 6) == 0) {
            *session_state = LOGGED_OUT;
            printf("Successful unregister.\n");
        } else if (strncmp(response, "RUR NOK", 7) == 0) {
            printf("User not logged in.\n");
        } else if (strncmp(response, "RUR UNR", 7) == 0) {
            printf("Unknown user.\n");
        } else if (strncmp(response, "RUR WRP", 7) == 0) {
            printf("Incorrect unregister attempt.\n");
        } else if (strncmp(response, "RUR ERR", 7) == 0) {
            printf("Sintax error in unregister.\n");
        }
    }
}

void handle_publish(const char *input, const char *uid, const char *password, char *filename, char *label, const char *dsip, const char *dsport, int *session_state) {
    char msg[512];
    char response[128];
    char extra[2];
    FILE *file;

    if (sscanf(input, "%*s %127s %127s %1s", filename, label, extra) != 2) {
        printf("Syntax error in publish.\n");
        return;
    }

    // Validate filename
    if (!valid_filename(filename)) {
        printf("Invalid filename.\n");
        return;
    }

    // Validate label
    if (!valid_label(label)) {
        printf("Invalid label.\n");
        return;
    }

    // check if file exists in directory
    file = fopen(filename, "rb");

    if (file == NULL) {
        printf("File not found.\n");
        return;
    }

    // get file size
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fclose(file);

    //check file size
    if (fsize > 10000000) {
        printf("File is too long.\n");
        return;
    }

    if (*session_state == LOGGED_OUT) {
        printf("User not logged in.\n");
        return;
    }

    snprintf(msg, sizeof(msg), "PUB %s %s %s %ld %s\n", uid, password, filename, fsize, label);

    if (send_receive_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
        if (strncmp(response, "RPB OK", 6) == 0) {
            printf("Successful publication.\n");
        } else if (strncmp(response, "RPB NLG", 7) == 0) {
            printf("User not logged in.\n");
        } else if (strncmp(response, "RPB UNR", 7) == 0) {
            printf("User not registered.\n");
        } else if (strncmp(response, "RPB WRP", 7) == 0) {
            printf("Incorrect password.\n");
        } else if (strncmp(response, "RPB NOK", 7) == 0) {
            printf("Unsuccessful publication.\n");
        } else if (strncmp(response, "RPB ERR", 7) == 0) {
            printf("Syntax error in publish.\n");
        }
    }
}

void handle_remove(const char *input, const char *uid, const char *password, char *filename, const char *dsip, const char *dsport, int *session_state) {
    char msg[512];
    char response[128];
    char extra[2];

    if (sscanf(input, "%*s %127s %1s", filename, extra) != 1) {
        printf("Syntax error in remove.\n");
        return;
    }

    // Validate filename
    if (!valid_filename(filename)) {
        printf("Invalid filename.\n");
        return;
    }

    if (*session_state == LOGGED_OUT) {
        printf("User not logged in.\n");
        return;
    }

    snprintf(msg, sizeof(msg), "REM %s %s %s\n", uid, password, filename);

    if (send_receive_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
        if (strncmp(response, "RRM OK", 6) == 0) {
            printf("Successful removal.\n");
        } else if (strncmp(response, "RRM NLG", 7) == 0) {
            printf("User not logged in.\n");
        } else if (strncmp(response, "RRM UNR", 7) == 0) {
            printf("User not registered.\n");
        } else if (strncmp(response, "RRM WRP", 7) == 0) {
            printf("Incorrect password.\n");
        } else if (strncmp(response, "RRM NOK", 7) == 0) {
            printf("Resource not found.\n");
        } else if (strncmp(response, "RRM ERR", 7) == 0) {
            printf("Syntax error in remove.\n");
        }
    }
}

void handle_list(const char *dsip, const char *dsport) {
    char msg[128];
    char response[512];

    snprintf(msg, sizeof(msg), "LST\n");

    if (send_receive_udp(dsip, dsport, msg, response, sizeof(response)) == 0) {
        if (strncmp(response, "RLS OK", 6) == 0) {
            printf("List of resources:\n");

            char *resources = response + 7;
            char *token = strtok(resources, " \n");

            while (token != NULL) {
                printf("%s\n", token);
                token = strtok(NULL, " \n");
            }
        } else if (strncmp(response, "RLS NOK", 7) == 0) {
            printf("No resources available.\n");
        } else if (strncmp(response, "RLS ERR", 7) == 0) {
            printf("Syntax error in list.\n");
        }
    }
}

void handle_versions(const char *input, char *filename, const char *dsip, const char *dsport) {
    char msg[512];
    char response[512];
    char extra[2];

    if (sscanf(input, "%*s %127s %1s", filename, extra) != 1) {
        printf("Syntax error in versions.\n");
        return;
    }

    if (!valid_filename(filename)) {
        printf("Invalid filename.\n");
        return;
    }

    snprintf(msg, sizeof(msg), "VRS %s\n", filename);

    if (send_receive_tcp(dsip, dsport, msg, response, sizeof(response)) == 0) {
        if (strncmp(response, "RVR OK", 6) == 0) {
            printf("Versions of the resource:\n");

            char *versions = response + 7;
            char *token = strtok(versions, " \n");

            while (token != NULL) {
                if (strcmp(token, "AVL") == 0) {
                    printf("Available\n");
                } else if (strcmp(token, "NAV") == 0) {
                    printf("Not available\n");
                } else {
                    printf("%s ", token);
                }

                token = strtok(NULL, " \n");
            }
        } else if (strncmp(response, "RVR NOK", 7) == 0) {
            printf("No peer available for such resource: %s.\n", filename);
        } else if (strncmp(response, "RVR ERR", 7) == 0) {
            printf("Syntax error in versions.\n");
        }
    }
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
    char filename[128];
    char label[21];
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