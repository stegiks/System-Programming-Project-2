#include "../include/helpserver.h"

List buffer_with_tasks = NULL;
uint32_t buffer_size;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;

/*
    Main function for multi-threaded server where arguments
    given are validated and then buffer and worker threads are created. 
*/
int main(int argc, char* argv[]){

    if(!validate_arguments(argc, argv)){
        print_error_and_die("jobExecutorServer : Invalid command line arguments\n"
        "Usage: ./jobExecutorServer [portnum] [buffersize] [threadPoolSize]\n");
    }

    uint16_t port = atoi(argv[1]);
    buffer_size = atoi(argv[2]);
    uint32_t threadPoolSize = atoi(argv[3]);

    // Create the buffer as a List and the worker threads
    buffer_with_tasks = createList(buffer_size);
    
    pthread_t worker_threads[threadPoolSize];
    for(int i = 0; i < threadPoolSize; i++)
        if(pthread_create(worker_threads[i], NULL, worker_function, NULL))
            print_error_and_die("jobExecutorServer : Error creating worker thread number %d", i);
    
    // Create socket, bind and listen
    int sock_server;
    create_connection(&sock_server, port);

    // While loop that accepts connections and creates controller threads
    while(1){
        int client_sock;
        if((client_sock = accept(sock_server, NULL, NULL)) < 0)
            print_error_and_die("jobExecutorServer : Error accepting connection");

        int* pclient_sock = malloc(sizeof(int));
        if(!pclient_sock)
            print_error_and_die("jobExecutorServer : Error allocating memory for pclient_sock");
        
        *pclient_sock = client_sock;

        pthread_t controller_thread;
        pthread_create(&controller_thread, NULL, controller_function, pclient_sock);
    }

    return 0;
}