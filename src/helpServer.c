#include "../include/helpserver.h"

uint32_t id = 1;
uint32_t concurrency = 1;

/*
    This function finds the number of digits in a number
*/
uint64_t find_digits(int num){
    uint64_t digits = 0;
    do {
        num /= 10;
        ++digits;
    } while (num != 0);

    return digits;
}

/*
    Get the metadata information of the job and put it in the buffer.
*/
void readCommand(int client_sock, void* buffer_start, void* buffer_ptr, uint32_t* length_of_message, uint32_t* number_of_args, size_t* length_of_command, char* command){
    if(m_read(client_sock, buffer_start) == -1)
        print_error_and_die("jobExecutorServer : Error reading message from client");
    
    // Get the info
    memcpy(length_of_message, buffer_ptr, sizeof(uint32_t));
    buffer_ptr += sizeof(uint32_t);

    memcpy(number_of_args, buffer_ptr, sizeof(uint32_t));
    buffer_ptr += sizeof(uint32_t);

    memcpy(length_of_command, buffer_ptr, sizeof(size_t));
    buffer_ptr += sizeof(size_t);

    command = (char*)malloc(*length_of_command + 1);
    if(!command)
        print_error_and_die("jobExecutorServer : Error allocating memory for command");
    
    memcpy(command, buffer_ptr, *length_of_command);
    command[*length_of_command] = '\0';
    buffer_ptr += *length_of_command;
}

/*
    Set the concurrency of the server and respond to the client.
*/
void setConcurrency(void* buffer, int client_sock){
    size_t length_of_concurrency;
    memcpy(&length_of_concurrency, buffer, sizeof(size_t));
    buffer += sizeof(size_t);

    char* new_concurrency = (char*)malloc(length_of_concurrency + 1);
    if(!new_concurrency)
        print_error_and_die("jobExecutorServer : Error allocating memory for new_concurrency");

    memcpy(new_concurrency, buffer, length_of_concurrency);
    new_concurrency[length_of_concurrency] = '\0';

    concurrency = atoi(new_concurrency);

    // Send the response : CONCURRENCY SET AT Ν
    uint32_t lenght_of_response = 19 + find_digits(concurrency);
    void* response_start = malloc(lenght_of_response + sizeof(uint32_t));
    if(!response_start)
        print_error_and_die("jobExecutorServer : Error allocating memory for response");
    
    void* response = response_start;
    memcpy(response, &lenght_of_response, sizeof(uint32_t));
    response += sizeof(uint32_t);

    // Construct the message
    char* message = malloc(lenght_of_response + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for message");
    
    sprintf(message, "CONCURRENCY SET AT %d", concurrency);
    message[lenght_of_response] = '\0';
    memcpy(response, message, lenght_of_response);

    // Write the response to the client
    if(m_write(client_sock, response_start, lenght_of_response + sizeof(uint32_t)) == -1)
        print_error_and_die("jobExecutorServer : Error writing response to client");
    
    free(new_concurrency);
    free(message);
    free(response_start);
}

/*
    Remove a job from the buffer and respond to the client.
*/
void stopJob(void* buffer, int client_sock){
    size_t length_of_jobid;
    memcpy(&length_of_jobid, buffer, sizeof(size_t));
    buffer += sizeof(size_t);

    char* jobid = (char*)malloc(length_of_jobid + 1);
    if(!jobid)
        print_error_and_die("jobExecutorServer : Error allocating memory for jobid");
    
    memcpy(jobid, buffer, length_of_jobid);
    jobid[length_of_jobid] = '\0';

    // Find the job in the shared buffer, so use the mutex for safety
    pthread_mutex_lock(&buffer_mutex);
    ListNode current = buffer_with_tasks->head;
    bool found = false;
    while(current != NULL){
        if(strcmp(current->job->jobid, jobid) == 0){
            removeJob(buffer_with_tasks, current->job);
            found = true;
            break;
        }
    }
    pthread_mutex_unlock(&buffer_mutex);

    // Send response depending on the result
    // Found : JOB <jobID> REMOVED
    // Not found : JOB <jobID> NOTFOUND
    uint32_t lenght_of_response = (found) ? (12 + length_of_jobid) : (13 + length_of_jobid);
    void* response_start = malloc(lenght_of_response + sizeof(uint32_t));
    if(!response_start)
        print_error_and_die("jobExecutorServer : Error allocating memory for response");
    
    void* response = response_start;
    memcpy(response, &lenght_of_response, sizeof(uint32_t));
    response += sizeof(uint32_t);

    // Construct message
    char* message = malloc(lenght_of_response + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for message");
    
    if(found)
        sprintf(message, "JOB %s REMOVED", jobid);
    else
        sprintf(message, "JOB %s NOTFOUND", jobid);
    
    message[lenght_of_response] = '\0';
    memcpy(response, message, lenght_of_response);

    if(m_write(client_sock, response_start, lenght_of_response + sizeof(uint32_t)) == -1)
        print_error_and_die("jobExecutorServer : Error writing response to client");
    
    free(jobid);
    free(message);
    free(response_start);
}

/*
    Poll the jobs that are currently queued in the 
    buffer and respond to the client.
*/
void pollJob(int client_sock){
    // Find the number of jobs in the buffer
    pthread_mutex_lock(&buffer_mutex);
    uint32_t number_of_jobs = buffer_with_tasks->size;
    pthread_mutex_unlock(&buffer_mutex);

    void* response_start = NULL;
    if(number_of_jobs == 0){
        // Send response : [number of jobs] NO JOBS IN BUFFER
        uint32_t lenght_of_response = 17 + sizeof(uint32_t);
        response_start = malloc(lenght_of_response + sizeof(uint32_t));
        if(!response_start)
            print_error_and_die("jobExecutorServer : Error allocating memory for response");
        
        void* response = response_start;
        memcpy(response, &lenght_of_response, sizeof(uint32_t));
        response += sizeof(uint32_t);

        memcpy(response, &number_of_jobs, sizeof(uint32_t));
        response += sizeof(uint32_t);

        // Construct message
        uint32_t length_of_message = 17;
        char* message = malloc(length_of_message + 1);
        if(!message)
            print_error_and_die("jobExecutorServer : Error allocating memory for message");
        
        sprintf(message, "NO JOBS IN BUFFER");
        message[length_of_message] = '\0';
        memcpy(response, message, length_of_message);

        if(m_write(client_sock, response_start, lenght_of_response + sizeof(uint32_t)) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");
        
        free(message);
    }
    else{
        // Send response : 
        // [length] [number of jobs] [length of jobid1] [jobid1] [length of job1] [job1] ...
        // Find length of the message
        uint32_t length_of_message = 0;

        pthread_mutex_lock(&buffer_mutex);
        ListNode current = buffer_with_tasks->head;
        while(current != NULL){
            length_of_message += 2 * sizeof(size_t) + strlen(current->job->jobid) + strlen(current->job->command);
            current = current->next;
        }

        length_of_message += sizeof(uint32_t);
        response_start = malloc(length_of_message + sizeof(uint32_t)); 
        if(!response_start)
            print_error_and_die("jobExecutorServer : Error allocating memory for response");
        
        void* response = response_start;
        memcpy(response, &length_of_message, sizeof(uint32_t));
        response += sizeof(uint32_t);

        memcpy(response, &number_of_jobs, sizeof(uint32_t));
        response += sizeof(uint32_t);

        current = buffer_with_tasks->head;
        while(current != NULL){
            size_t length_of_jobid = strlen(current->job->jobid);
            size_t length_of_command = strlen(current->job->command);

            memcpy(response, &length_of_jobid, sizeof(size_t));
            response += sizeof(size_t);

            memcpy(response, current->job->jobid, length_of_jobid);
            response += length_of_jobid;

            memcpy(response, &length_of_command, sizeof(size_t));
            response += sizeof(size_t);

            memcpy(response, current->job->command, length_of_command);
            response += length_of_command;

            current = current->next;
        }
        pthread_mutex_unlock(&buffer_mutex);

        if(m_write(client_sock, response_start, length_of_message + sizeof(uint32_t)) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");
        
    }

    free(response_start);
}


/*
    This function handles controller threads. They read the 
    message from the client and then put it in the buffer.
*/
void* controller_function(void* arg){
    int client_sock = (*(int*)arg);
    free(arg);

    void* buffer_start = NULL;
    void* buffer_ptr = buffer_start;
    uint32_t length_of_message;
    uint32_t number_of_args;
    size_t length_of_command;
    char* command = NULL;
    readCommand(client_sock, buffer_start, buffer_ptr, &length_of_message, &number_of_args, &length_of_command, command);

    // Handle each command differently
    if(strcmp(command, "issueJob") == 0){   // TODO
        issueJob(buffer_ptr, client_sock, length_of_message, number_of_args);
    }
    else if(strcmp(command, "setConcurrency") == 0){
        setConcurrency(buffer_ptr, client_sock);
    }
    else if(strcmp(command, "stop") == 0){
        stopJob(buffer_ptr, client_sock);
    }
    else if(strcmp(command, "poll") == 0){
        pollJob(client_sock);
    }
    else{   // exit // TODO
        exitServer(client_sock);
    }

    if(m_close(client_sock) == -1)
        print_error_and_die("jobExecutorServer : Error closing client socket");
    
    free(command);
    free(buffer_start);
    return NULL;
}


/*
    This function is used to validate the arguments given in the command line.
*/
bool validate_arguments(int argc, char** argv){
    if(argc != 4)
        return false;
    
    if(!isNumber(argv[1]) || !isNumber(argv[2]) || !isNumber(argv[3]))
        return false;
    
    int port = atoi(argv[1]);
    int buffersize = atoi(argv[2]);
    int threadPoolSize = atoi(argv[3]);

    // Check for valid numbers
    if(port < 0 || port > 65535 || buffersize < 1 || threadPoolSize < 1)
        return false;

    return true;
}

/*
    This function is used to create a socket for the server,
    specify the configuration of the socket and then bind and listen.
*/
void create_connection(int* sock_server, uint16_t port){

    if((*sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        print_error_and_die("jobExecutorServer : Error creating socket for server");
    
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(*sock_server, (struct sockaddr*)&server, sizeof(server)) < 0)
        print_error_and_die("jobExecutorServer : Error binding socket to address");
    
    if(listen(*sock_server, MAX_CONNECTIONS) < 0)
        print_error_and_die("jobExecutorServer : Error listening for connections");

}