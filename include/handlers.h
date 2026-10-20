#ifndef HANDLERS_H
#define HANDLERS_H

#include "includes.h"

#define LOGGED_OUT 0
#define LOGGED_IN 1

void handle_login(const char *input, char *uid, char *password, int tcpport, const char *dsip, const char *dsport, int *session_state, int *fd);
void handle_logout(const char *uid, const char *password, const char *dsip, const char *dsport, int *session_state, int *fd);
void handle_unregister(const char *uid, const char *password, const char *dsip, const char *dsport, int *session_state, int *fd);
void handle_publish(const char *input, const char *uid, const char *password, char *filename, char *label, const char *dsip, const char *dsport, int *session_state, int *fd);
void handle_remove(const char *input, const char *uid, const char *password, char *filename, const char *dsip, const char *dsport, int *session_state, int *fd);
void handle_list(const char *dsip, const char *dsport, int *fd); 
void handle_versions(const char *input, char *filename, const char *dsip, const char *dsport, char *uid_peer);
void handle_get(const char *input, char *filename, const char *uid_peer, const char *dsip, const char *dsport, int *fd);

#endif // HANDLERS_H
