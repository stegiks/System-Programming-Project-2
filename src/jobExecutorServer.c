#include "../include/helpserver.h"

// Global variables and Queue-Buffer
bool terminate = false;
int worker_threads_working = 0;
int active_controller_threads = 0;
int concurrency = 1;
Queue buffer_with_tasks = NULL;
uint32_t buffer_size;
uint32_t thread_pool_size;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t global_vars_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t worker_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t controller_cond = PTHREAD_COND_INITIALIZER;

/*
    Main function for multi-threaded server where arguments
    given are validated and then buffer and worker threads are created. 
*/
int main(int argc, char* argv[]){

    if(!validate_arguments(argc, argv)){
        printf(BLUE "Usage: " RESET "jobExecutorServer [port] [buffer_size] [thread_pool_size]\n");
        print_error_and_die("jobExecutorServer : Invalid command line arguments\n");
    }

    uint16_t port = atoi(argv[1]);
    buffer_size = atoi(argv[2]);
    thread_pool_size = atoi(argv[3]);

    // Create the buffer as a Queue and the worker threads
    buffer_with_tasks = createQueue(buffer_size);
    
    pthread_t* worker_trheads;
    worker_trheads = malloc(thread_pool_size * sizeof(pthread_t));
    for(uint32_t i = 0; i < thread_pool_size; i++)
        if(pthread_create(&worker_trheads[i], NULL, worker_function, NULL))
            print_error_and_die("jobExecutorServer : Error creating worker thread number %d", i);
    
    // Create socket, bind and listen
    int sock_server;
    if(!create_connection(&sock_server, port)){
        // Possibly Address already in use
        free(worker_trheads);
        cleanQueue(buffer_with_tasks);
        print_error_and_die("jobExecutorServer : Error creating connection");
    }

    // While loop that accepts connections and creates controller threads.
    // Before that it checks for termination.
    printf(GREEN "Server is running\n" RESET);
    while(1){

        pthread_mutex_lock(&terminate_mutex);
        if(terminate){
            if(active_controller_threads > 0)
                pthread_cond_wait(&controller_cond, &terminate_mutex);
            
            pthread_mutex_unlock(&terminate_mutex);
            break;
        }
        pthread_mutex_unlock(&terminate_mutex);

        // Use accept to get a client socket
        int client_sock;
        if((client_sock = accept(sock_server, NULL, NULL)) < 0){

            // If accept fails, check for termination
            pthread_mutex_lock(&terminate_mutex);
            if((errno == EINVAL) && terminate){
                if(active_controller_threads > 0)
                    pthread_cond_wait(&controller_cond, &terminate_mutex);

                pthread_mutex_unlock(&terminate_mutex);
                break;
            }
            pthread_mutex_unlock(&terminate_mutex);

            if(errno == EINTR)
                continue;
            else{
                print_error_and_die("jobExecutorServer : Error accepting connection");
            }
        }

        // Gets freed in controller_function immediately
        int* sockets = malloc(sizeof(int) * 2);
        if(!sockets)
            print_error_and_die("jobExecutorServer : Error allocating memory for pclient_sock");
        
        sockets[0] = client_sock;
        sockets[1] = sock_server;

        pthread_t controller_thread;
        if(pthread_create(&controller_thread, NULL, controller_function, sockets))
            print_error_and_die("jobExecutorServer : Error creating controller thread");
        
        if(pthread_detach(controller_thread))
            print_error_and_die("jobExecutorServer : Error detaching controller thread");
    }


    // Join worker threads
    for(uint32_t i = 0; i < thread_pool_size; i++){
        if(pthread_join(worker_trheads[i], NULL))
            print_error_and_die("jobExecutorServer : Error joining worker thread number %d", i);

        printf("Worker thread number %d joined\n", i);
    }

    // Free worker threads array
    free(worker_trheads);

    // Clean up : queue, mutexes and condition variables
    termination();

    printf(RED "Server is terminating\n" RESET);

    return 0;
}