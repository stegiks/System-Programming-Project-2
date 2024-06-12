#include "../include/helpcommander.h"

/*
    Main function where user input is validated 
    and sent to the server for processing
*/
int main(int argc, char* argv[]){

    struct addrinfo *server_info = NULL;

    if(!validate_command(argc, argv, &server_info)){ 
        usage_commander();
        print_error_and_die("Invalid command line arguments");
    }

    // Create a connection using the given hostname and port
    int sock;
    login(&sock, &server_info, argv[2]);

    // Free the memory allocated for server_info
    freeaddrinfo(server_info);

    send_command(sock, argc, argv);

    recieve_response(sock, argv[3]);

    return 0;
}