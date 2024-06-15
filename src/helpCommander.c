#include "../include/helpcommander.h"


/*
    This function prints the file to the standard output.
*/
void print_file(char* file){
    int file_descriptor;
    if((file_descriptor = m_open(file, O_RDONLY)) == -1)
        print_error_and_die("jobCommander : Error opening the file %s", file);
    
    char buffer[CHUNKSIZE];
    ssize_t bytes_read;
    while((bytes_read = read(file_descriptor, buffer, CHUNKSIZE)) > 0)
        write(STDOUT_FILENO, buffer, bytes_read);
    
    if(bytes_read == -1)
        print_error_and_die("jobCommander : Error reading the file %s", file);
    
    printf("\n\n");
    
    if(m_close(file_descriptor) == -1)
        print_error_and_die("jobCommander : Error closing the file %s", file);
}


/*
    This function initializes the file names for the output
    and response files. The file names are in the format
    /tmp/res_pid.txt and /tmp/out_pid.txt
*/
char* init_file_names(const char* pid, const char* type){
    // type can be either res or out
    size_t length = strlen("/tmp/") + strlen(type) + strlen(pid) + strlen(".txt") + 1;
    char* file = malloc(length + 1);
    if(!file)
        print_error_and_die("jobCommander : Error allocating memory for file");
    
    file[length] = '\0';
    sprintf(file, "/tmp/%s_%s.txt", type, pid);
    return file;
}


/*
    This function prints the usage of the jobCommander.
    It prints the available commands and their description.
*/
void usage_commander() {
    printf(BLUE "Usage:" RESET " ./jobCommander [serverName] [portNum] [jobCommanderInputCommand]\n\n");
    printf(GREEN "Available commands are:\n\n" RESET);

    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET MAGENTA "1. issueJob <job>            " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET YELLOW "Send a job for execution to  " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "the server, where job is a   " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "UNIX like command.           " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n\n" RESET);

    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET MAGENTA "2. setConcurrency <N>        " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET YELLOW "Set the maximum number of    " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "jobs that can be run         " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "concurrently to N (N > 0).   " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n\n" RESET);

    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET MAGENTA "3. stop <jobID>              " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET YELLOW "Stop (remove it from         " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "the buffer) the job with the " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "given jobID.                 " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n\n" RESET);

    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET MAGENTA "4. poll                      " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET YELLOW "Print the job's ID and the   " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "job itself for all jobs in   " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "the buffer.                  " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n\n" RESET);

    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET MAGENTA "5. exit                      " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n" RESET);
    printf(CYAN "| " RESET YELLOW "Terminate the                " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "jobExecutorServer which      " RESET CYAN "|\n" RESET);
    printf(CYAN "| " RESET YELLOW "handles all the jobs.        " RESET CYAN "|\n" RESET);
    printf(CYAN "+------------------------------+\n\n" RESET);
}


/*
    This function checks if the command line arguments are valid.
    It must check if the hostname resolves to an IP address,
    if the port number is valid one and if the command is one of
    those that are supported by the jobCommander.
*/
bool validate_command(int argc, char** argv, struct addrinfo** server_info){
    if(argc < 4){
        perror("jobCommander : Invalid number of arguments\n");
        return false;
    }

    // Check if the port number is valid. 0 to 65535
    if(!isNumber(argv[2])){
        perror("jobCommander : Port number must be a number\n");
        return false;
    }

    int port = atoi(argv[2]);
    if(port < 0 || port > 65535)
        return false;

    // Check if the hostname resolves to an IP address
    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_CANONNAME;
    
    if(getaddrinfo(argv[1], argv[2], &hints, server_info) != 0){
        fprintf(stderr, "jobCommander : Hostname %s does not resolve to an IP address\n", argv[1]);
        return false;
    }

    // Check if the command is one of those that are supported by the jobCommander
    char* command = argv[3];

    if((strcmp(command, "issueJob") == 0)){
        if(argc < 5){
            perror("jobCommander : issueJob command must have a job\n");
            return false;
        }
    }
    else if(strcmp(command, "setConcurrency") == 0){
        if(argc < 5){
            perror("jobCommander : setConcurrency command must have a number\n");
            return false;
        }
        
        if(!isNumber(argv[4])){
            perror("jobCommander : setConcurrency command must have a number\n");
            return false;
        }
        
        int N = atoi(argv[4]);
        if(N < 1){
            perror("jobCommander : N must be greater than 0\n");
            return false;
        }
    }
    else if(strcmp(command, "stop") == 0){
        if(argc != 5){
            perror("jobCommander : stop command must have a jobID\n");
            return false;
        }
    }
    else if((strcmp(command, "poll") == 0) || (strcmp(command, "exit") == 0)){
        if(argc != 4){
            fprintf(stderr, "jobCommander : %s command must not have any arguments\n", command);
            return false;
        }
    }
    else{
        fprintf(stderr, "jobCommander : %s command is not supported\n", command);
        return false;
    }

    return true;
}


/*
    This function is used from client to make a connection
    with the server using socket programming. 
*/
void login(int* sock, struct addrinfo** server_info, char* port){
    
    struct sockaddr *serverptr = (struct sockaddr *)(*server_info)->ai_addr;

    // Create socket
    if((*sock = socket((*server_info)->ai_family, (*server_info)->ai_socktype, (*server_info)->ai_protocol)) < 0)
        print_error_and_die("jobCommander : Error creating a socket");     

    // Initiate connection
    if(connect(*sock, serverptr, (*server_info)->ai_addrlen) < 0)
        print_error_and_die("jobCommander : Error connecting to the server with hostname %s and port %s\n", (*server_info)->ai_canonname, port);
}


/*
    Send the command to the server for processing.
    The writing format is: 
    [length of message] [num of args] [length of 1st command] [1st command] ...
    This is done repeatedly until the whole command is sent.
*/
void send_command(int sock, int argc, char** argv){
    
    uint32_t command_length = 0;
    for(int i = 3; i < argc; i++)
        command_length += sizeof(size_t) + strlen(argv[i]);
    
    // Allocate memory for the message = command length + number of arguments
    uint32_t message_length = sizeof(uint32_t) + command_length;
    void* message = malloc(message_length);
    if(!message)
        print_error_and_die("jobCommander : Error allocating memory for the message");
    

    // Copy the number of arguments first
    void* temp = message;
    uint32_t actual_number_of_args = argc - 4;
    memcpy(temp, &actual_number_of_args, sizeof(uint32_t));
    temp += sizeof(uint32_t);
    
    for(int i = 3; i < argc; i++){
        size_t length = strlen(argv[i]);
        memcpy(temp, &length, sizeof(size_t));
        temp += sizeof(size_t);
        memcpy(temp, argv[i], length);
        temp += length;
    }

    uint32_t total_length = sizeof(uint32_t) + sizeof(uint32_t) + command_length;
    if((m_write(sock, message, total_length)) == -1)
        print_error_and_die("jobCommander : Error writing the message to the server");

    free(message);
}


/*
    This function reads the response from the server.
    The client is responsible to get message response
    from the server and wait for the result of the UNIX 
    command if the command was issueJob.
*/
void recieve_response(int sock, char* command){

    if((strcmp(command, "exit") == 0) || (strcmp(command, "stop") == 0) || (strcmp(command, "setConcurrency") == 0)){
        void* buffer = NULL;
        if(m_read(sock, &buffer) == -1)
            print_error_and_die("jobCommander : Error reading the EXIT message from the server");
        
        // Get pass the length of the message
        uint32_t length_of_message;
        memcpy(&length_of_message, buffer, sizeof(uint32_t));
        
        char* response = malloc(length_of_message - sizeof(uint32_t) + 1);
        memcpy(response, buffer + sizeof(uint32_t), length_of_message - sizeof(uint32_t));
        response[length_of_message - sizeof(uint32_t)] = '\0';

        printf("%s\n", response);
        free(response);
        free(buffer);
    }
    else if(strcmp(command, "poll") == 0){
        // Read the number of jobs first
        uint32_t number_of_jobs;
        if(read(sock, &number_of_jobs, sizeof(uint32_t)) == -1)
            print_error_and_die("jobCommander : Error reading the number of jobs from the server");

        void* buffer = NULL;
        if(number_of_jobs == 0){
            if(m_read(sock, &buffer) == -1)
                print_error_and_die("jobCommander : Error reading the POLL message from the server");
            
            // Get pass the length of the message
            uint32_t length_of_buffer;
            memcpy(&length_of_buffer, buffer, sizeof(uint32_t));
        
            // Length of the message is the length of the buffer - bytes for the length
            uint32_t length_of_message = length_of_buffer - sizeof(uint32_t);
            char* response = malloc(length_of_message + 1);
            memcpy(response, buffer + sizeof(uint32_t), length_of_message);
            response[length_of_message] = '\0';

            printf("%s\n", response);
            free(response);
            free(buffer);
        }
        else{
            for(uint32_t i = 0; i < number_of_jobs; i++){
                // JOBID 
                if(m_read(sock, &buffer) == -1)
                    print_error_and_die("jobCommander : Error reading the POLL message from the server");
                
                uint32_t length_of_buffer;
                memcpy(&length_of_buffer, buffer, sizeof(uint32_t));

                // Length of the message is the length of the buffer - bytes for the length
                uint32_t length_of_jobid = length_of_buffer - sizeof(uint32_t);
                char* jobid = malloc(length_of_jobid + 1);
                if(!jobid)
                    print_error_and_die("jobCommander : Error allocating memory for jobid");
                
                memcpy(jobid, buffer + sizeof(uint32_t), length_of_jobid);
                jobid[length_of_jobid] = '\0';

                // Print the jobid and clean up
                printf("<%s , ", jobid);
                memsetzero_and_free(buffer, length_of_buffer);
                memsetzero_and_free((void*)jobid, length_of_jobid);

                // JOB : Job may be coming in chunks. 
                // For example job "ls -l /usr/bin/* /usr/local/bin/*" has a big length
                while(true){
                    if(m_read(sock, &buffer) == -1)
                        print_error_and_die("jobCommander : Error reading the POLL message from the server");
                    
                    uint32_t length_of_buffer;
                    memcpy(&length_of_buffer, buffer, sizeof(uint32_t));

                    // Length of the message is the length of the buffer - bytes for the length - bytes for the type
                    uint32_t length_of_job = length_of_buffer - sizeof(uint32_t) - sizeof(int);

                    // Get the type of message
                    int type;
                    memcpy(&type, buffer + sizeof(uint32_t), sizeof(int));

                    // Get the message
                    char* message = malloc(length_of_job + 1);
                    if(!message)
                        print_error_and_die("jobCommander : Error allocating memory for message");
                    
                    memcpy(message, buffer + sizeof(uint32_t) + sizeof(int), length_of_job);
                    message[length_of_job] = '\0';
                    
                    // Check type to handle
                    if(type == 1){
                        // Response chunk
                        printf("%s", message);
                    }
                    else{
                        // End of response
                        printf("%s>\n", message);
                        memsetzero_and_free(buffer, length_of_buffer);
                        memsetzero_and_free((void*)message, length_of_job);
                        break;
                    }

                    memsetzero_and_free(buffer, length_of_buffer);
                    memsetzero_and_free((void*)message, length_of_job);
                }
            }
        }
    }
    else{
        // IssueJob command
        // Find the pid of the process and make pid string
        pid_t pid = getpid();
        uint64_t digits = find_digits(pid);
        char* pid_string = malloc(digits + 1);
        if(!pid_string)
            print_error_and_die("jobCommander : Error allocating memory for pid_string");
        
        sprintf(pid_string, "%d", pid);

        // Initialize the file names and open the files
        char* file_response = init_file_names(pid_string, "res");
        char* file_output = init_file_names(pid_string, "out");
    
        int fd_res;
        if((fd_res = m_open(file_response, O_CREAT | O_RDWR | O_APPEND)) == -1)
            print_error_and_die("jobCommander : Error opening the file %s", file_response);
        
        int fd_out;
        if((fd_out = m_open(file_output, O_CREAT | O_RDWR | O_APPEND)) == -1)
            print_error_and_die("jobCommander : Error opening the file %s", file_output);
        
        bool output_done = false, res_done = false, terminated = false;

        // Read the response from the server. Response may be coming in many chunks
        while((!(output_done) || !(res_done)) && !(terminated)){
            void* buffer = NULL;
            if(m_read(sock, &buffer) == -1)
                print_error_and_die("jobCommander : Error reading the response from the server");
            
            uint32_t length_of_buffer;
            memcpy(&length_of_buffer, buffer, sizeof(uint32_t));

            // Length of the message is the length of the buffer - bytes for the length - bytes for the type
            uint32_t length_of_message = length_of_buffer - sizeof(uint32_t) - sizeof(int);

            // Get the type of message
            int type;
            memcpy(&type, buffer + sizeof(uint32_t), sizeof(int));

            // Get the message
            char* message = (char*)(buffer + sizeof(uint32_t) + sizeof(int));

            // Check type to handle
            if(type == 1){
                // Response chunk
                if(write(fd_res, message, length_of_message) == -1)
                    print_error_and_die("jobCommander : Error writing to the file %s", file_response);
            }
            else if(type == 2){
                // Output chunk
                if(write(fd_out, message, length_of_message) == -1)
                    print_error_and_die("jobCommander : Error writing to the file %s", file_output);
            }
            else if(type == -1){
                // End of response
                if(write(fd_res, message, length_of_message) == -1)
                    print_error_and_die("jobCommander : Error writing to the file %s", file_response);
                
                res_done = true;
                print_file(file_response);
            }
            else if(type == -2){
                // End of output
                if(write(fd_out, message, length_of_message) == -1)
                    print_error_and_die("jobCommander : Error writing to the file %s", file_output);
                
                output_done = true;
                print_file(file_output);
            }
            else if(type == 0){
                // Termination before submitting the job
                terminated = true;
                char* termination_message = malloc(length_of_message + 1);
                if(!termination_message)
                    print_error_and_die("jobCommander : Error allocating memory for termination_message");
                
                memcpy(termination_message, message, length_of_message);
                termination_message[length_of_message] = '\0';

                printf("%s\n", termination_message);
                free(termination_message);
            }

            // Memset to 0 before freeing
            memsetzero_and_free(buffer, length_of_buffer);

        }

        // Close the files and remove them
        if(m_close(fd_res) == -1)
            print_error_and_die("jobCommander : Error closing the file %s", file_response);
        
        if(m_close(fd_out) == -1)
            print_error_and_die("jobCommander : Error closing the file %s", file_output);

        remove(file_response);
        remove(file_output);
        free(file_response);
        free(file_output);
        free(pid_string);
    }

    if(m_close(sock) == -1)
        print_error_and_die("jobCommander : Error closing the socket");
}