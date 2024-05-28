#include "../include/helpcommander.h"

/*
    Main function where user input is validated 
    and sent to the server for processing
*/
int main(int argc, char* argv[]){

    struct hostnet* server_info = NULL;

    if(!validate_command(argc, argv, server_info)){
        usage();
        print_error_and_die("Invalid command line arguments");
    }

    // Create a connection using the given hostname and port
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    int sock;
    login(&sock, &server, serverptr, server_info, argv[2]);

    send_command(sock, argc, argv);

    recieve_response(sock);

    close(sock);

    return 0;
}

// TODO : implement recieve_response
// TODO : implement help functions like m_read, m_close
// TODO : after that start with the server side