#include "../include/helpserver.h"

bool terminate = false;
int active_controller_threads = 0;
int worker_threads_working = 0;
List buffer_with_tasks = NULL;
pthread_t* worker_trheads;
uint32_t buffer_size;
uint32_t thread_pool_size;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t global_vars_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t worker_cond = PTHREAD_COND_INITIALIZER;

/*
    Main function for multi-threaded server where arguments
    given are validated and then buffer and worker threads are created. 
*/
int main(int argc, char* argv[]){

    if(!validate_arguments(argc, argv)){
        print_error_and_die("jobExecutorServer : Invalid command line arguments\n"
        "Usage: ./jobExecutorServer [portnum] [buffersize] [threadPoolSize]\n");
    }
    printf("Arguments are valid\n");

    uint16_t port = atoi(argv[1]);
    buffer_size = atoi(argv[2]);
    thread_pool_size = atoi(argv[3]);

    // Create the buffer as a List and the worker threads
    buffer_with_tasks = createList(buffer_size);
    
    worker_trheads = malloc(thread_pool_size * sizeof(pthread_t));
    for(uint32_t i = 0; i < thread_pool_size; i++)
        if(pthread_create(&worker_trheads[i], NULL, worker_function, NULL))
            print_error_and_die("jobExecutorServer : Error creating worker thread number %d", i);
    
    printf("Buffer and worker threads created\n");
    // Create socket, bind and listen
    int sock_server;
    create_connection(&sock_server, port);

    printf("Server listening on port %d\n", port);

    // While loop that accepts connections and creates controller threads.
    // Before that it checks for termination.
    printf("Server is ready to accept connections\n");
    while(1){

        pthread_mutex_lock(&terminate_mutex);
        if(terminate){
            if(active_controller_threads > 0)
                pthread_cond_wait(&terminate_cond, &terminate_mutex);
            
            pthread_mutex_unlock(&terminate_mutex);
            break;
        }
        pthread_mutex_unlock(&terminate_mutex);

        int client_sock;
        printf("Waiting for connection...\n");
        if((client_sock = accept(sock_server, NULL, NULL)) < 0)
            print_error_and_die("jobExecutorServer : Error accepting connection");

        // Gets freed in controller_function immediately
        int* pclient_sock = malloc(sizeof(int));
        if(!pclient_sock)
            print_error_and_die("jobExecutorServer : Error allocating memory for pclient_sock");
        
        *pclient_sock = client_sock;

        pthread_t controller_thread;
        if(pthread_create(&controller_thread, NULL, controller_function, pclient_sock))
            print_error_and_die("jobExecutorServer : Error creating controller thread");
        
        if(pthread_detach(controller_thread))
            print_error_and_die("jobExecutorServer : Error detaching controller thread");
    }

    // Check if some controller threads managed to put 
    // their jobs in the buffer after exit command was sent.
    termination();

    printf("I AM YOUR FATHER...\n");
    printf("NOOOOO\n");

    return 0;
}