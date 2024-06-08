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
    printf("Connecting to server...\n");
    login(&sock, &server_info, argv[2]);
    printf("Connected to server\n");

    // Free the memory allocated for server_info
    printf("Freeing server_info\n");
    freeaddrinfo(server_info);
    printf("Freed server_info\n");

    printf("Sending command to server\n");
    send_command(sock, argc, argv);
    printf("Sent command to server\n");

    printf("Recieving response from server\n");
    recieve_response(sock, argv[3]);
    printf("Recieved response from server\n");

    return 0;
}