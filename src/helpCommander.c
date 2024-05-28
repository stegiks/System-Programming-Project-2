#include "../include/helpcommander.h"

/*
    This functions checks if a string is a number.
*/
bool isNumber(char *str){
    for(int i = 0; i < strlen(str); i++)
        if(!isdigit(str[i]))
            return false;
        
    return true;
}

/*
    This function prints the usage of the jobCommander.
    It prints the available commands and their description.
*/
void usage(){
    int white_space1 = 10;
    int white_space2 = 20;
    printf("Usage : ./jobCommander [serverName] [portNum] [jobCommanderInputCommand]\n\n");
    printf("Available commands are:\n\n");
    printf("%*s%s", white_space1, "", "1. issueJob <job> :\n");
    printf("%*s%s", white_space2, "", "Send a job for execution to the server, where job is a UNIX like command\n\n");
    printf("%*s%s", white_space1, "", "2. setConcurrency <N>\n");
    printf("%*s%s", white_space2, "", "Set the maximum number of jobs that can be run concurrently to N (N > 0)\n\n");
    printf("%*s%s", white_space1, "", "3, stop <jobID> :\n");
    printf("%*s%s", white_space2, "", "Stop (remove it from the buffer) the job with the given jobID\n\n");
    printf("%*s%s", white_space1, "", "4. poll :\n");
    printf("%*s%s", white_space2, "", "Print the job's ID and the job itself for all jobs in the buffer\n\n");
    printf("%*s%s", white_space1, "", "5. exit :\n");
    printf("%*s%s", white_space2, "", "Terminates the jobExecutorServer which handles all the jobs\n\n");
}


/*
    This function checks if the command line arguments are valid.
    It must check if the hostname resolves to an IP address,
    if the port number is valid one and if the command is one of
    those that are supported by the jobCommander.
*/
bool validate_command(int argc, char* argv[], struct hostnet* server){
    if(argc < 4){
        return false;
    }

    // Check if the hostname resolves to an IP address
    if((server = gethostbyname(argv[1])) == NULL)
        return false;

    // Check if the port number is valid. 0 to 65535
    uint32_t port = atoi(argv[2]);
    if(port < 0 || port > 65535)
        return false;
    
    // Check if the command is one of those that are supported by the jobCommander
    char* command = argv[3];

    if((strcmp(command, "issueJob") == 0) && (argc < 5)){
        return false;
    }
    else if(strcmp(command, "setConcurrency") == 0){
        if(argc < 5)
            return false;
        
        if(!isNumber(argv[4]))
            return false;
        
        int N = atoi(argv[4]);
        if(N < 1)
            return false;
    }
    else if(strcmp(command, "stop") == 0){
        if(argc != 5)
            return false;
    }
    else if((strcmp(command, "poll") == 0) || (strcmp(command, "exit") == 0)){
        if(argc != 4)
            return false;
    }

    return true;
}

/*
    This function is used from client to make a connection
    with the server using socket programming. 
*/
void login(int* sock, struct sockaddr_in* server, struct sockaddr* serverptr, struct hostent* server_info, char* port){
    
    // Create socket
    if((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        print_error_and_die("jobCommander : Error creating a socket");

    server->sin_family = AF_INET;
    memcpy(&server->sin_addr, server_info->h_addr, server_info->h_length);
    server->sin_port = htons(atoi(port));         

    // Initiate connection
    if(connect(*sock, serverptr, sizeof(*server)) < 0)
        print_error_and_die("jobCommander : Error connecting to the server with hostname %s and port %s\n", server_info->h_name, port);

    printf("jobCommander : Connecting to %s port %s\n", server_info->h_name, port);
}

/*
    Send the command to the server for processing.
    The writing format is: 
    [length of message] [num of args] [command length] [length of 1st command] [1st command] ...
    This is done repeatedly until the whole command is sent.
*/
void send_command(int sock, int argc, char** argv){
    
    // Find how many bytes the command will take
    uint32_t command_length = 0;
    for(int i = 3; i < argc; i++)
        command_length += sizeof(size_t) + strlen(argv[i]);
    
    // Allocate memory for the message
    uint32_t actual_number_of_args = argc - 3;
    uint32_t total_length = sizeof(uint32_t) + sizeof(uint32_t) + command_length;
    void* message = malloc(total_length);
    if(!message)
        print_error_and_die("jobCommander : Error allocating memory for the message");
    
    // Construct the message for writing
    void* temp = message;
    memcpy(temp, &total_length, sizeof(uint32_t));
    temp += sizeof(uint32_t);
    memcpy(temp, &actual_number_of_args, sizeof(uint32_t));
    temp += sizeof(uint32_t);
    memcpy(temp, &command_length, sizeof(uint32_t));
    temp += sizeof(uint32_t);
    for(int i = 3; i < argc; i++){
        size_t length = strlen(argv[i]);
        memcpy(temp, &length, sizeof(size_t));
        temp += sizeof(size_t);
        memcpy(temp, argv[i], length);
        temp += length;
    }

    // Write the message to the server
    if((m_write(sock, message, total_length)) == -1)
        print_error_and_die("jobCommander : Error writing the message to the server");

    free(message);
}