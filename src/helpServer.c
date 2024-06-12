#include "../include/helpserver.h"

/*
    This function is used to validate the arguments given in the command line.
*/
bool validate_arguments(int argc, char** argv){
    if(argc != 4)
        return false;
    
    if(!isNumber(argv[1]) || !isNumber(argv[2]) || !isNumber(argv[3]))
        return false;
    
    int port = atoi(argv[1]);
    int buffersize = atoi(argv[2]);
    int threadPoolSize = atoi(argv[3]);

    // Check for valid numbers
    if(port < 0 || port > 65535 || buffersize < 1 || threadPoolSize < 1)
        return false;   

    return true;
}

/*
    This function is used to create a socket for the server,
    specify the configuration of the socket and then bind and listen.
*/
bool create_connection(int* sock_server, uint16_t port){

    if((*sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return false;
    
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(*sock_server, (struct sockaddr*)&server, sizeof(server)) < 0)
        return false;
    
    if(listen(*sock_server, MAX_CONNECTIONS) < 0)
        return false;
    
    return true;
}

/*
    Free the memory allocated for the buffer,
    mutexes and condition variables.
*/
void termination(){
    cleanQueue(buffer_with_tasks);

    // Destroy mutexes and condition vars
    pthread_mutex_destroy(&buffer_mutex);
    pthread_mutex_destroy(&terminate_mutex);
    pthread_mutex_destroy(&global_vars_mutex);
    pthread_cond_destroy(&buffer_not_empty);
    pthread_cond_destroy(&buffer_not_full);
    pthread_cond_destroy(&worker_cond);
    pthread_cond_destroy(&controller_cond);

}