#include "../include/helpserver.h"

uint32_t id = 1;

/*
    This function writes the message but with a int type in front.
    Useful for functions like issueJob, stopJob, pollJob who want
    to tell server what type of message is recieving.
*/
void sendMessageWithType(int fd, const char* message, int type, uint32_t size_message){
    // Allocate memory for message and type
    void* response = malloc(size_message + sizeof(int));
    if(!response)
        print_error_and_die("jobExecutorServer : Error allocating memory for response, sendMessageWithType");
    
    memcpy(response, &type, sizeof(int));
    memcpy(response + sizeof(int), message, size_message);

    uint32_t total_size = size_message + sizeof(int) + sizeof(uint32_t);
    if(m_write(fd, response, total_size) == -1)
        print_error_and_die("jobExecutorServer : Error writing response to client");
    
    memsetzero_and_free(response, size_message + sizeof(int));
}

/*
    Get the metadata information of the job and put it in the buffer.
*/
void readCommand(int client_sock, void** buffer_start, void** buffer_ptr, uint32_t* length_of_message, uint32_t* number_of_args, size_t* length_of_command, char** command){
    if(m_read(client_sock, buffer_start) == -1)
        print_error_and_die("jobExecutorServer : Error reading message from client");
    
    *buffer_ptr = *buffer_start;
    if(*buffer_ptr == NULL)
        print_error_and_die("jobExecutorServer : TEO");
    
    // Get the info
    memcpy(length_of_message, *buffer_ptr, sizeof(uint32_t));
    (*buffer_ptr) += sizeof(uint32_t);

    memcpy(number_of_args, *buffer_ptr, sizeof(uint32_t));
    *buffer_ptr += sizeof(uint32_t);

    memcpy(length_of_command, *buffer_ptr, sizeof(size_t));
    *buffer_ptr += sizeof(size_t);

    // Command : issueJob, setConcurrency, stop, poll, exit
    *command = (char*)malloc(*length_of_command + 1);
    if(!(*command))
        print_error_and_die("jobExecutorServer : Error allocating memory for command");
    
    memcpy(*command, (*buffer_ptr), *length_of_command);
    *buffer_ptr += *length_of_command;
    (*command)[*length_of_command] = '\0';
}

/*
    Construct the arguments array for the job.
*/
char** constructArgsArray(void* buffer, uint32_t number_of_args, char** command){
    char** arguments = (char**)malloc((number_of_args + 1) * sizeof(char*));
    if(!arguments)
        print_error_and_die("jobExecutorServer : Error allocating memory for arguments");
    
    uint32_t length_of_whole_command = 0;
    // Arguments array
    for(uint32_t i = 0; i < number_of_args; i++){
        size_t length_of_arg;
        memcpy(&length_of_arg, buffer, sizeof(size_t));
        buffer += sizeof(size_t);

        arguments[i] = (char*)malloc(length_of_arg + 1);
        if(!(arguments[i]))
            print_error_and_die("jobExecutorServer : Error allocating memory for arguments[%d]", i);
        
        memcpy(arguments[i], buffer, length_of_arg);
        arguments[i][length_of_arg] = '\0';
        buffer += length_of_arg;

        length_of_whole_command += length_of_arg + 1;
    }

    // NULL terminate the arguments array
    arguments[number_of_args] = NULL;

    // Construct the whole command
    *command = (char*)malloc(length_of_whole_command);
    if(!(*command))
        print_error_and_die("jobExecutorServer : Error allocating memory for command");
    
    char* temp = *command;
    for(uint32_t i = 0; i < number_of_args; i++){
        strncpy(temp, arguments[i], strlen(arguments[i]));
        temp += strlen(arguments[i]);

        // Add space or NULL terminator
        if(i != number_of_args - 1){
            strncpy(temp, " ", 1);
            temp++;
        }
        else{
            strncpy(temp, "\0", 1);
        }
    }

    return arguments;
}

/*
    Issue a job to the buffer and respond to the client.
*/
void issueJob(void* buffer, int client_sock, uint32_t number_of_args, ThreadData m_thread_data){
    // Create a new job for the Queue-Buffer
    Job job = (Job)malloc(sizeof(*job));

    // Job ID
    pthread_mutex_lock(&global_vars_mutex);
    uint64_t digits = find_digits(id);
    job->jobid = (char*)malloc(digits + 5);
    sprintf(job->jobid, "job_%d", id);
    job->jobid[digits + 4] = '\0';

    id++;
    pthread_mutex_unlock(&global_vars_mutex);

    // Number of arguments and file descriptor
    job->number_of_args = number_of_args;
    job->fd = client_sock;

    // Thread data. Make it point to the same address as the argument
    job->thread_data = m_thread_data;

    // Arguments array and whole command
    job->arguments = constructArgsArray(buffer, job->number_of_args, &(job->command));

    // Check if the buffer is full. If it is wait.
    pthread_mutex_lock(&buffer_mutex);
    while(isFull(buffer_with_tasks))
        pthread_cond_wait(&buffer_not_full, &buffer_mutex);
    
    pthread_mutex_unlock(&buffer_mutex);

    // Check for termination after potential broadcast by exitServer
    pthread_mutex_lock(&terminate_mutex);
    if(terminate){
        char message[35] = "SERVER TERMINATED BEFORE EXECUTION";
        message[34] = '\0';
        int type = 0;

        uint32_t size_to_write = (uint32_t)strlen(message);
        sendMessageWithType(client_sock, message, type, size_to_write);
        
        // Tell controller_function not to wait for worker response
        pthread_mutex_lock(&job->thread_data->mutex);
        job->thread_data->worker_response_ready = true;
        pthread_cond_signal(&job->thread_data->cond);
        pthread_mutex_unlock(&job->thread_data->mutex);

        // Clean up
        free(job->jobid);
        free(job->command);
        for(uint32_t i = 0; i < job->number_of_args; i++)
            free(job->arguments[i]);
        
        free(job->arguments);
        free(job);
        pthread_mutex_unlock(&terminate_mutex);
        return;
    }
    pthread_mutex_unlock(&terminate_mutex);
    
    // Enqueue the job
    pthread_mutex_lock(&buffer_mutex);
    enqueue(buffer_with_tasks, job);
    pthread_cond_signal(&buffer_not_empty);
    pthread_mutex_unlock(&buffer_mutex);

    // Send response : JOB <jobID, job> SUBMITTED
    uint32_t length_of_response = 18 + strlen(job->jobid) + strlen(job->command);

    // Construct the message
    char* message = malloc(length_of_response + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for message");

    sprintf(message, "JOB <%s, %s> SUBMITTED", job->jobid, job->command);
    message[length_of_response] = '\0';

    // We need to write the response in chunks just like in the worker function
    uint32_t bytes_written = 0;
    uint32_t bytes_left = length_of_response;
    while(bytes_written < length_of_response){
        
        // Type to indicate if the chunk is final or not
        uint32_t size_to_write;
        int type;

        // Set the size to write and the type of chunk
        if(bytes_left > CHUNKSIZE){
            size_to_write = CHUNKSIZE;
            type = 1;
        }
        else{
            size_to_write = bytes_left;
            type = -1;
        }

        sendMessageWithType(client_sock, message + bytes_written, type, size_to_write);
        
        bytes_written += size_to_write;
        bytes_left -= size_to_write;
    }
    
    free(message);
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

    pthread_mutex_lock(&global_vars_mutex);
    int old_concurrency = concurrency;
    concurrency = atoi(new_concurrency);

    // Signal the worker threads to check the new concurrency
    if(old_concurrency < concurrency)
        pthread_cond_broadcast(&worker_cond);

    // Send the response : CONCURRENCY SET AT Ν
    uint32_t lenght_of_response = 19 + find_digits(concurrency);
    char* message = malloc(lenght_of_response + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for message");
    
    sprintf(message, "CONCURRENCY SET AT %d", concurrency);
    pthread_mutex_unlock(&global_vars_mutex);
    message[lenght_of_response] = '\0';

    // Write the response to the client
    if(m_write(client_sock, (void*)message, lenght_of_response + sizeof(uint32_t)) == -1)
        print_error_and_die("jobExecutorServer : Error writing response to client");
    
    free(new_concurrency);
    free(message);
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
    QueueNode current = buffer_with_tasks->head;
    bool found = false;
    while(current != NULL){
        QueueNode temp = current->next;
        if(strcmp(current->job->jobid, jobid) == 0){

            // Send client : JOB REMOVED BEFORE EXECUTION BY STOP COMMAND
            int type = -2;
            char message[45] = "JOB REMOVED BEFORE EXECUTION BY STOP COMMAND";
            message[44] = '\0';

            uint32_t size_to_write = (uint32_t)strlen(message);
            sendMessageWithType(current->job->fd, message, type, size_to_write);
            
            // Let the controller thread know that it doesn't have to wait for the worker response
            pthread_mutex_lock(&(current->job->thread_data->mutex));
            current->job->thread_data->worker_response_ready = true;
            pthread_cond_signal(&(current->job->thread_data->cond));
            pthread_mutex_unlock(&(current->job->thread_data->mutex));

            // Remove the job from the buffer and signal controller threads
            removeJob(buffer_with_tasks, current->job->jobid);
            pthread_cond_signal(&buffer_not_full);
            found = true;
            break;
        }
        current = temp;
    }
    pthread_mutex_unlock(&buffer_mutex);

    // Send response depending on the result
    // Found : JOB <jobID> REMOVED
    // Not found : JOB <jobID> NOTFOUND
    uint32_t lenght_of_message = (found) ? (12 + length_of_jobid) : (13 + length_of_jobid);

    // Construct message
    char* message = malloc(lenght_of_message + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for message");
    
    if(found)
        sprintf(message, "JOB %s REMOVED", jobid);
    else
        sprintf(message, "JOB %s NOTFOUND", jobid);
    
    message[lenght_of_message] = '\0';

    if(m_write(client_sock, (void*)(message), lenght_of_message + sizeof(uint32_t)) == -1)
        print_error_and_die("jobExecutorServer : Error writing response to client");
    
    free(jobid);
    free(message);
}

/*
    Poll the jobs that are currently queued in the 
    buffer and respond to the client.
*/
void pollJob(int client_sock){
    
    // First we find the number of jobs in the buffer 
    pthread_mutex_lock(&buffer_mutex);
    uint32_t number_of_jobs = buffer_with_tasks->size;

    if(number_of_jobs == 0){
        // Here we can release the mutex because we are not going to change the buffer
        pthread_mutex_unlock(&buffer_mutex);

        // Send response : [number of jobs] NO JOBS IN BUFFER
        if(write(client_sock, &number_of_jobs, sizeof(uint32_t)) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");

        // Construct message
        uint32_t length_of_message = 17;
        char* message = malloc(length_of_message + 1);
        if(!message)
            print_error_and_die("jobExecutorServer : Error allocating memory for message");
        
        sprintf(message, "NO JOBS IN BUFFER");
        message[length_of_message] = '\0';

        uint32_t total_size = length_of_message + sizeof(uint32_t);
        if(m_write(client_sock, (void*)(message), total_size) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");
        
        free(message);
    }
    else{
        // Send response : [number of jobs] [length of jobid1] [jobid1] [length of job1] [job1] ...
        if(write(client_sock, &number_of_jobs, sizeof(uint32_t)) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");
        
        QueueNode current = buffer_with_tasks->head;
        while(current != NULL){
            
            // Write the jobid
            uint32_t length_of_jobid = (uint32_t)strlen(current->job->jobid);
            if(m_write(client_sock, current->job->jobid, length_of_jobid + sizeof(uint32_t)) == -1)
                print_error_and_die("jobExecutorServer : Error writing response to client");
            
            // Job command must be written in chunks of CHUNKSIZE
            // because it may be too big.
            uint32_t bytes_written = 0;
            uint32_t bytes_left = (uint32_t)strlen(current->job->command);
            while(bytes_written < strlen(current->job->command)){
                uint32_t size_to_write;
                int type;

                // Set the size to write and the type of chunk
                if(bytes_left > CHUNKSIZE){
                    size_to_write = CHUNKSIZE;
                    type = 1;
                }
                else{
                    size_to_write = bytes_left;
                    type = -1;
                }

                sendMessageWithType(client_sock, current->job->command + bytes_written, type, size_to_write);
                
                bytes_written += size_to_write;
                bytes_left -= size_to_write;
            }
            
            current = current->next;

        }
        pthread_mutex_unlock(&buffer_mutex);
    }
}

/*
    Exit the server and respond to the client. Server will 
    terminate after all jobs that are running are finished.
    It will remove the jobs from the buffer and send a response
    for the clients waiting : SERVER TERMINATED BEFORE EXECUTION 
*/
void exitServer(int client_sock){

    // Send response : SERVER TERMINATED
    uint32_t length_of_message = 17;
    char* message = malloc(length_of_message + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for message");
    
    sprintf(message, "SERVER TERMINATED");
    message[17] = '\0';

    uint32_t total_size = length_of_message + sizeof(uint32_t);
    if(m_write(client_sock, (void*)(message), total_size) == -1)
        print_error_and_die("jobExecutorServer : Error writing response to client");
    
    memsetzero_and_free(message, length_of_message);

    // Set the terminate flag to true and broadcast to all threads for termination check
    pthread_mutex_lock(&terminate_mutex);
    terminate = true;
    pthread_cond_broadcast(&buffer_not_empty);
    pthread_cond_broadcast(&buffer_not_full);
    pthread_cond_broadcast(&worker_cond);
    pthread_mutex_unlock(&terminate_mutex);

    // Iterate through the buffer, remove the jobs and send response
    pthread_mutex_lock(&buffer_mutex);
    QueueNode current = buffer_with_tasks->head;
    // length_of_message = 34;
    while(current != NULL){

        // Send response : SERVER TERMINATED BEFORE EXECUTION
        int type = -2;
        char message[35] = "SERVER TERMINATED BEFORE EXECUTION";
        message[34] = '\0';

        uint32_t size_to_write = (uint32_t)strlen(message);
        sendMessageWithType(current->job->fd, message, type, size_to_write);

        // Let the controller thread know that it doesn't have 
        // to wait for the worker response
        pthread_mutex_lock(&(current->job->thread_data->mutex));
        current->job->thread_data->worker_response_ready = true;
        pthread_cond_signal(&(current->job->thread_data->cond));
        pthread_mutex_unlock(&(current->job->thread_data->mutex));

        QueueNode temp = current;
        current = current->next;
        removeJob(buffer_with_tasks, temp->job->jobid);
    }
    pthread_mutex_unlock(&buffer_mutex);
}


/*
    This function handles controller threads. They read the 
    message from the client and then put it in the buffer.
*/
void* controller_function(void* arg){
    // Get the client socket and the server socket
    int* sockets = (int*)arg;
    int client_sock = sockets[0];
    int sock_server = sockets[1];
    free(arg);

    pthread_mutex_lock(&terminate_mutex);
    active_controller_threads++;
    pthread_mutex_unlock(&terminate_mutex);

    // Read the message from the client
    void* buffer_start = NULL;
    void* buffer_ptr = buffer_start;
    uint32_t length_of_message;
    uint32_t number_of_args;
    size_t length_of_command;
    char* command = NULL;
    readCommand(client_sock, &buffer_start, &buffer_ptr, &length_of_message, &number_of_args, &length_of_command, &command);

    // Handle each command differently
    ThreadData thread_data = NULL;
    if(strcmp(command, "issueJob") == 0){

        thread_data = (ThreadData)malloc(sizeof(*thread_data));
        if(!thread_data)
            print_error_and_die("jobExecutorServer : Error allocating memory for thread_data");
         
        // Used for closing client socket
        thread_data->worker_response_ready = false;

        if(pthread_mutex_init(&(thread_data->mutex), NULL) != 0)
            print_error_and_die("jobExecutorServer : Error initializing mutex");
        
        if(pthread_cond_init(&(thread_data->cond), NULL) != 0)
            print_error_and_die("jobExecutorServer : Error initializing condition variable");
        
        issueJob(buffer_ptr, client_sock, number_of_args, thread_data);
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
    else{   // exit 
        exitServer(client_sock);
    }

    // Check if the worker has finished writing 
    // the response to close the socket and free the memory
    if(thread_data != NULL){
        pthread_mutex_lock(&(thread_data->mutex));
        while(!(thread_data->worker_response_ready))
            pthread_cond_wait(&(thread_data->cond), &(thread_data->mutex));
        
        pthread_mutex_unlock(&(thread_data->mutex));

        pthread_mutex_destroy(&(thread_data->mutex));
        pthread_cond_destroy(&(thread_data->cond));
        free(thread_data);
    }

    // Free the memory and close the socket
    if(m_close(client_sock) == -1)
        print_error_and_die("jobExecutorServer : Error closing client socket");


    // Check for termination and close the server socket
    pthread_mutex_lock(&terminate_mutex);
    if((terminate) && (strcmp(command, "exit") == 0)){
        pthread_mutex_unlock(&terminate_mutex);
        
        // Close the socket with shutdown
        if(shutdown(sock_server, SHUT_RD) == -1)
            print_error_and_die("jobExecutorServer : Error shutting down server socket");
    }
    else{
        pthread_mutex_unlock(&terminate_mutex);
    }

    free(command);
    free(buffer_start);

    pthread_mutex_lock(&terminate_mutex);
    active_controller_threads--;
    if((terminate) && (active_controller_threads == 0))
        pthread_cond_signal(&controller_cond);

    pthread_mutex_unlock(&terminate_mutex);

    return NULL;
}