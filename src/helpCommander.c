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
    This function reads the poll message from the server.
    The format will be : 
    [length of jobID] [jobID] [length of job] [job]
*/
void read_poll_message(void* buffer, char** jobid, char** job){
    uint32_t length_of_jobid;
    memcpy(&length_of_jobid, buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);
    *jobid = malloc(length_of_jobid + 1);
    if(!(*jobid))
        print_error_and_die("jobCommander : Error allocating memory for jobid");
    
    memcpy(*jobid, buffer, length_of_jobid);
    (*jobid)[length_of_jobid] = '\0';
    buffer += length_of_jobid;

    uint32_t length_of_job;
    memcpy(&length_of_job, buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);
    *job = malloc(length_of_job + 1);
    if(!(*job))
        print_error_and_die("jobCommander : Error allocating memory for job");
    
    memcpy(*job, buffer, length_of_job);
    (*job)[length_of_job] = '\0';
    buffer += length_of_job;
}


/*
    This function prints the usage of the jobCommander.
    It prints the available commands and their description.
*/
void usage_commander(){
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
bool validate_command(int argc, char** argv, struct addrinfo* server_info){
    if(argc < 4){
        return false;
    }

    // Specify the type of the address
    struct addrinfo hints;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    // Check if the hostname resolves to an IP address
    if(getaddrinfo(argv[1], NULL, &hints, &server_info) != 0)
        return false;

    // Check if the port number is valid. 0 to 65535
    if(!isNumber(argv[2]))
        return false;

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
void login(int* sock, struct addrinfo* server_info, char* port){
    
    struct sockaddr *serverptr = (struct sockaddr *)server_info->ai_addr;

    // Create socket
    if((*sock = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol)) < 0)
        print_error_and_die("jobCommander : Error creating a socket");     

    // Initiate connection
    if(connect(*sock, serverptr, server_info->ai_addrlen) < 0)
        print_error_and_die("jobCommander : Error connecting to the server with hostname %s and port %s\n", server_info->h_name, port);

    printf("jobCommander : Connecting to %s port %s\n", server_info->ai_canonname, port);
}

/*
    Send the command to the server for processing.
    The writing format is: 
    [length of message] [num of args] [length of 1st command] [1st command] ...
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
    void* message = malloc(total_length + sizeof(uint32_t));
    if(!message)
        print_error_and_die("jobCommander : Error allocating memory for the message");
    
    // Construct the message for writing
    void* temp = message;
    memcpy(temp, &total_length, sizeof(uint32_t));
    temp += sizeof(uint32_t);
    memcpy(temp, &actual_number_of_args, sizeof(uint32_t));
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

/*
    This function reads the response from the server.
    The client is responsible to get message response
    from the server and wait for the result of the UNIX 
    command if the command was issueJob.
*/
void recieve_response(int sock, char* command){
    
    void* buffer;

    if((strcmp(command, "exit") == 0) || (strcmp(command, "stop") == 0) || (strcmp(command, "setConcurrency") == 0)){
        if(m_read(sock, buffer) == -1)
            print_error_and_die("jobCommander : Error reading the EXIT message from the server");
        
        // Get pass the length of the message
        char* response = (char*)(buffer + sizeof(uint32_t));
        printf("jobCommander : %s\n", response);
    }
    else if(strcmp(command, "poll") == 0){
        if(m_read(sock, buffer) == -1)
            print_error_and_die("jobCommander : Error reading the POLL message from the server");
        
        // Parse the buffer and print the results
        // The format will be : 
        // [length_of_message] [number of jobs] [length of jobID] [jobID] [length of job] [job] ... OR
        // [length_of_message] [number of jobs] [message]
        void* buffer_ptr = buffer;
        uint32_t length_of_message;
        memcpy(&length_of_message, buffer_ptr, sizeof(uint32_t));
        buffer_ptr += sizeof(uint32_t);

        uint32_t number_of_jobs;
        memcpy(&number_of_jobs, buffer_ptr, sizeof(uint32_t));
        buffer_ptr += sizeof(uint32_t);
        printf("jobCommander : POLL\n");

        if(number_of_jobs == 0){
            uint32_t len_response = length_of_message - sizeof(uint32_t);
            char* response = malloc(len_response + 1);
            if(!response)
                print_error_and_die("jobCommander : Error allocating memory for response");
            
            memcpy(response, buffer, len_response);
            response[len_response] = '\0';
            
            printf("%s\n", response);
            free(response);
        }
        else{
            for(int i = 0; i < number_of_jobs; i++){
                char* jobid;
                char* job;

                read_poll_message(buffer_ptr, &jobid, &job);
                printf("<%s , %s>\n", jobid, job);
                
                free(jobid);
                free(job);
            }
        }

    }
    else{
        // Format will be : [length of chunk] [chunk] ...
        char* worker_response;
        int size_of_worker_response = 0;
        uint16_t both = 0;
        while(1){
            if(m_read(sock, buffer) == -1)
                print_error_and_die("jobCommander : Error reading the response from the server");
            
            // Get the length of the chunk
            uint32_t length_of_chunk;
            void* buffer_ptr = buffer;
            memcpy(&length_of_chunk, buffer_ptr, sizeof(uint32_t));
            buffer_ptr += sizeof(uint32_t);

            // Allocate memory for the chunk
            char* chunk = malloc(length_of_chunk + 1);
            if(!chunk)
                print_error_and_die("jobCommander : Error allocating memory for chunk");
            
            // Copy the chunk to the buffer
            memcpy(chunk, buffer_ptr, length_of_chunk);
            chunk[length_of_chunk] = '\0';

            if(strncmp(chunk, "JOB <", 5) == 0){
                // Controller thread response
                printf("%s\n", chunk);
                both++;
                if(both == 2){
                    free(chunk);
                    break;
                }
            }
            else{
                // Chunk from worker thread. Reallocate for worker_response
                size_of_worker_response += length_of_chunk;
                worker_response = realloc(worker_response, size_of_worker_response);
                if(!worker_response)
                    print_error_and_die("jobCommander : Error reallocating memory for worker_response");
                
                strcat(worker_response, chunk);
            }

            if(strncmp(chunk, "-----", 5) == 0){
                // Worker thread response
                worker_response = realloc(worker_response, size_of_worker_response + 1);
                if(!worker_response)
                    print_error_and_die("jobCommander : Error reallocating memory for worker_response");
                
                worker_response[size_of_worker_response] = '\0';
                printf("%s\n", worker_response);
                free(worker_response);

                both++;
                if(both == 2){
                    free(chunk);
                    break;
                }
            }

            free(chunk);
            // ! MALLON FINITO !
        }

    }

    // If allocation wasn't successful then the 
    // program will have already exited
    free(buffer);

    if(m_close(sock) == -1)
        print_error_and_die("jobCommander : Error closing the socket");
}