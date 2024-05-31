#ifndef HELPSERVER_H
#define HELPSERVER_H

#include "listjobs.h"
#define MAX_CONNECTIONS 100

List buffer_with_tasks;
uint32_t buffer_size;
pthread_mutex_t buffer_mutex;
pthread_cond_t buffer_not_full;
pthread_cond_t buffer_not_empty;

void* worker_function(void* arg);
void* controller_function(void* arg);
bool validate_arguments(int argc, char** argv);
void create_connection(int* sock_server, uint16_t port);

#endif /* helpserver.h */