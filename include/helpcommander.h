#ifndef HELPCOMMANDER_H
#define HELPCOMMANDER_H

#include "help.h"

void usage();
bool validate_command(int argc, char** argv, struct hostnet* server);
void login(int* sock, struct sockaddr_in* server, struct sockaddr* serverptr, struct hostnet* server_info, char* port);
void send_command(int sock, int argc, char** argv);
void recieve_response(int sock);

#endif /* helpcommander.h */