#include "../include/defs.h"

/*
    Main function for multi-threaded server where arguments
    given are validated and then buffer and worker threads are created. 
*/
int main(int argc, char* argv[]){

    if(!validate_arguments(argc, argv)){
        print_error_and_die("jobExecutorServer : Invalid command line arguments\n"
        "Usage: ./jobExecutorServer [portnum] [buffersize] [threadPoolSize]\n");
    }

    return 0;
}