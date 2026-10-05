#include "handlers.h"

#include <stdio.h>
#include <string.h>

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
             char *versions = response + 7;
            char *token = strtok(versions, " \n");

            printf("%-8s %-10s %-12s %-22s %s\n",
                "UID", "FSize", "Label", "Publication Time", "Status");

            while (token != NULL) {

                char *uid = token;
                char *fsize = strtok(NULL, " \n");
                char *label = strtok(NULL, " \n");
                char *publication_time = strtok(NULL, " \n");
                char *availability = strtok(NULL, " \n");

                if (fsize == NULL || label == NULL ||
                    publication_time == NULL || availability == NULL) {
                    break;
                }

                printf("%-8s %-10s %-12s %-22s %s\n",
                    uid, fsize, label, publication_time, availability);

                token = strtok(NULL, " \n");
            }
        } else if (strncmp(response, "RVR NOK", 7) == 0) {
            printf("No peer available for such resource: %s.\n", filename);
        } else if (strncmp(response, "RVR ERR", 7) == 0) {
            printf("Syntax error in versions.\n");
        }
    }
}