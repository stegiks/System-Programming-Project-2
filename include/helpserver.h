#ifndef HELPSERVER_H
#define HELPSERVER_H

#include "listjobs.h"
#define MAX_CONNECTIONS 100

extern bool terminate; 
extern int worker_threads_working;
extern int active_controller_threads;
extern List buffer_with_tasks;
extern uint32_t buffer_size;
extern uint32_t thread_pool_size;
extern pthread_mutex_t buffer_mutex;
extern pthread_mutex_t terminate_mutex;
extern pthread_mutex_t global_vars_mutex;
extern pthread_cond_t buffer_not_full;
extern pthread_cond_t buffer_not_empty;
extern pthread_cond_t worker_cond;
extern pthread_cond_t controller_cond;

void* controller_function(void* arg);
void* worker_function();
bool validate_arguments(int argc, char** argv);
void create_connection(int* sock_server, uint16_t port);
void termination();

#endif /* helpserver.h */