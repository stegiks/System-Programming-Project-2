#ifndef HELPCOMMANDER_H
#define HELPCOMMANDER_H

#include "help.h"

void usage_commander();
bool validate_command(int argc, char** argv, struct addrinfo* server);
void login(int* sock, struct addrinfo* server_info, char* port);
void send_command(int sock, int argc, char** argv);
void recieve_response(int sock, char* command);

#endif /* helpcommander.h */