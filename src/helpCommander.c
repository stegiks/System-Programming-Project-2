#include "../include/helpcommander.h"

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
bool validate_command(int argc, char** argv, struct addrinfo** server_info){
    if(argc < 4){
        printf("jobCommander : Invalid number of arguments\n");
        return false;
    }

    // Check if the port number is valid. 0 to 65535
    if(!isNumber(argv[2])){
        printf("jobCommander : Port number must be a number\n");
        return false;
    }

    int port = atoi(argv[2]);
    if(port < 0 || port > 65535)
        return false;


    struct addrinfo hints;
    bzero(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_CANONNAME;
    printf("jobCommander : Checking if hostname resolves to an IP address\n");
    
    // Check if the hostname resolves to an IP address
    if(getaddrinfo(argv[1], argv[2], &hints, server_info) != 0){
        printf("jobCommander : Hostname %s does not resolve to an IP address\n", argv[1]);
        return false;
    }

    // Check if the command is one of those that are supported by the jobCommander
    char* command = argv[3];

    if((strcmp(command, "issueJob") == 0) && (argc < 5)){
        printf("jobCommander : issueJob command must have a job\n");
        return false;
    }
    else if(strcmp(command, "setConcurrency") == 0){
        if(argc < 5){
            printf("jobCommander : setConcurrency command must have a number\n");
            return false;
        }
        
        if(!isNumber(argv[4])){
            printf("jobCommander : setConcurrency command must have a number\n");
            return false;
        }
        
        int N = atoi(argv[4]);
        if(N < 1){
            printf("jobCommander : N must be greater than 0\n");
            return false;
        }
    }
    else if(strcmp(command, "stop") == 0){
        if(argc != 5){
            printf("jobCommander : stop command must have a jobID\n");
            return false;
        }
    }
    else if((strcmp(command, "poll") == 0) || (strcmp(command, "exit") == 0)){
        if(argc != 4){
            printf("jobCommander : %s command must not have any arguments\n", command);
            return false;
        }
    }

    printf("jobCommander : Valid command\n");
    return true;
}

/*
    This function is used from client to make a connection
    with the server using socket programming. 
*/
void login(int* sock, struct addrinfo** server_info, char* port){
    
    struct sockaddr *serverptr = (struct sockaddr *)(*server_info)->ai_addr;

    // Create socket
    printf("jobCommander : Creating a socket\n");
    if((*sock = socket((*server_info)->ai_family, (*server_info)->ai_socktype, (*server_info)->ai_protocol)) < 0)
        print_error_and_die("jobCommander : Error creating a socket");     

    printf("jobCommander : Socket created\n");

    // Initiate connection
    if(connect(*sock, serverptr, (*server_info)->ai_addrlen) < 0)
        print_error_and_die("jobCommander : Error connecting to the server with hostname %s and port %s\n", (*server_info)->ai_canonname, port);

    printf("jobCommander : Connecting to %s port %s\n", (*server_info)->ai_canonname, port);
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
    
    printf("Total length = %d\n", total_length);
    printf("Command length = %d\n", command_length);
    // Construct the message for writing
    void* temp = message;
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
    printf("jobCommander : Writing the message to the server\n");
    printf("Socket for communication : %d\n", sock);
    if((m_write(sock, message, total_length)) == -1)
        print_error_and_die("jobCommander : Error writing the message to the server");

    printf("jobCommander : Message sent to the server\n");
    free(message);
}

/*
    This function prints the file to the standard output.
*/
void print_file(char* file){
    FILE* f = fopen(file, "r");
    if(!f)
        print_error_and_die("jobCommander : Error opening the file %s", file);
    
    char c;
    while((c = fgetc(f)) != EOF)
        printf("%c", c);
    
    printf("\n");
    
    fclose(f);
}

/*
    This function initializes the file names for the output
    and response files. The file names are in the format
    /tmp/res_pid.txt and /tmp/out_pid.txt
*/
char* init_file_names(const char* pid, const char* type){
    size_t length = strlen("/tmp/") + strlen(type) + strlen(pid) + strlen(".txt") + 1;
    char* file = malloc(length + 1);
    if(!file)
        print_error_and_die("jobCommander : Error allocating memory for file");
    
    file[length] = '\0';
    sprintf(file, "/tmp/%s_%s.txt", type, pid);
    return file;
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
        printf("jobCommander : Length of message %d\n", length_of_message);
        
        char* response = malloc(length_of_message - sizeof(uint32_t) + 1);
        memcpy(response, buffer + sizeof(uint32_t), length_of_message - sizeof(uint32_t));
        response[length_of_message - sizeof(uint32_t)] = '\0';

        printf("%s\n", response);
        free(response);
        free(buffer);
    }
    else if(strcmp(command, "poll") == 0){
        uint32_t number_of_jobs;
        if(read(sock, &number_of_jobs, sizeof(uint32_t)) == -1)
            print_error_and_die("jobCommander : Error reading the number of jobs from the server");

        void* buffer = NULL;
        if(number_of_jobs == 0){
            if(m_read(sock, &buffer) == -1)
                print_error_and_die("jobCommander : Error reading the POLL message from the server");
            
            // Get pass the length of the message
            uint32_t length_of_message;
            memcpy(&length_of_message, buffer, sizeof(uint32_t));
            printf("jobCommander : Length of message %d\n", length_of_message);
        
            char* response = malloc(length_of_message - sizeof(uint32_t) + 1);
            memcpy(response, buffer + sizeof(uint32_t), length_of_message - sizeof(uint32_t));
            response[length_of_message - sizeof(uint32_t)] = '\0';

            printf("%s\n", response);
            free(response);
            free(buffer);
        }
        else{
            for(uint32_t i = 0; i < number_of_jobs; i++){
                //! START JOBID
                if(m_read(sock, &buffer) == -1)
                    print_error_and_die("jobCommander : Error reading the POLL message from the server");
                
                uint32_t length_of_jobid;
                memcpy(&length_of_jobid, buffer, sizeof(uint32_t));
                length_of_jobid -= sizeof(uint32_t);

                printf("Length of jobid = %d\n", length_of_jobid);
                char* jobid = malloc(length_of_jobid + 1);
                if(!jobid)
                    print_error_and_die("jobCommander : Error allocating memory for jobid");
                
                memcpy(jobid, buffer + sizeof(uint32_t), length_of_jobid);
                jobid[length_of_jobid] = '\0';
                //! END JOBID

                printf("<%s , ", jobid);
                memset(buffer, 0, length_of_jobid + sizeof(uint32_t));
                memset(jobid, 0, length_of_jobid);
                free(jobid);
                free(buffer);

                //! START JOB
                // job may be coming in chunks
                while(true){
                    if(m_read(sock, &buffer) == -1)
                        print_error_and_die("jobCommander : Error reading the POLL message from the server");
                    
                    // Get the length of the message
                    uint32_t length_of_job;
                    memcpy(&length_of_job, buffer, sizeof(uint32_t));
                    length_of_job = length_of_job - sizeof(uint32_t) - sizeof(int);

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
                        memset(buffer, 0, length_of_job + sizeof(uint32_t) + sizeof(int));
                        memset(message, 0, length_of_job);
                        free(message);
                        free(buffer);
                        break;
                    }

                    // Memset to 0 before freeing
                    memset(buffer, 0, length_of_job + sizeof(uint32_t) + sizeof(int));
                    memset(message, 0, length_of_job);
                    free(message);
                    free(buffer);
                }
            }
        }
    }
    else{
        pid_t pid = getpid();
        uint64_t digits = find_digits(pid);
        char* pid_string = malloc(digits + 1);
        if(!pid_string)
            print_error_and_die("jobCommander : Error allocating memory for pid_string");
        
        sprintf(pid_string, "%d", pid);

        // Initialize the file names
        char* file_response = init_file_names(pid_string, "res");
        char* file_output = init_file_names(pid_string, "out");
    
        int fd_res;
        if((fd_res = m_open(file_response, O_CREAT | O_RDWR | O_APPEND)) == -1)
            print_error_and_die("jobCommander : Error opening the file %s", file_response);
        
        int fd_out;
        if((fd_out = m_open(file_output, O_CREAT | O_RDWR | O_APPEND)) == -1)
            print_error_and_die("jobCommander : Error opening the file %s", file_output);
        
        bool output_done = false, res_done = false, terminated = false;

        while((!(output_done) || !(res_done)) && !(terminated)){
            void* buffer = NULL;
            if(m_read(sock, &buffer) == -1)
                print_error_and_die("jobCommander : Error reading the response from the server");
            
            // Get the length of the message
            uint32_t length_of_message;
            memcpy(&length_of_message, buffer, sizeof(uint32_t));

            // Get type of message
            int type;
            memcpy(&type, buffer + sizeof(uint32_t), sizeof(int));

            // Get the message
            char* message = (char*)(buffer + sizeof(uint32_t) + sizeof(int));

            // Check type to handle
            if(type == 1){
                // Response chunk
                if(write(fd_res, message, length_of_message - sizeof(int) - sizeof(uint32_t)) == -1)
                    print_error_and_die("jobCommander : Error writing to the file %s", file_response);
            }
            else if(type == 2){
                // Output chunk
                if(write(fd_out, message, length_of_message - sizeof(int) - sizeof(uint32_t)) == -1)
                    print_error_and_die("jobCommander : Error writing to the file %s", file_output);
            }
            else if(type == -1){
                // End of response
                if(write(fd_res, message, length_of_message - sizeof(int) - sizeof(uint32_t)) == -1)
                    print_error_and_die("jobCommander : Error writing to the file %s", file_response);
                
                res_done = true;
                print_file(file_response);
            }
            else if(type == -2){
                // End of output
                if(write(fd_out, message, length_of_message - sizeof(int) - sizeof(uint32_t)) == -1)
                    print_error_and_die("jobCommander : Error writing to the file %s", file_output);
                
                output_done = true;
                print_file(file_output);
            }
            else if(type == 0){
                // Termination before submitting the job
                terminated = true;
                uint32_t length = length_of_message - sizeof(int) - sizeof(uint32_t);
                char* termination_message = malloc(length + 1);
                if(!termination_message)
                    print_error_and_die("jobCommander : Error allocating memory for termination_message");
                
                memcpy(termination_message, message, length);
                termination_message[length] = '\0';
                printf("%s\n", termination_message);
                free(termination_message);
            }

            // Memset to 0 before freeing
            memset(buffer, 0, length_of_message);
            free(buffer);

        }

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