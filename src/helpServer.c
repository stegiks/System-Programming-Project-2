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
    Construct the arguments array for the job.
*/
void constructArgsArray(void* buffer, uint32_t number_of_args, char** arguments, char* command){
    arguments = (char**)malloc((number_of_args + 1) * sizeof(char*));
    if(!arguments)
        print_error_and_die("jobExecutorServer : Error allocating memory for arguments");
    
    uint32_t length_of_whole_command;
    // Arguments array
    for(int i = 0; i < number_of_args; i++){
        size_t length_of_arg;
        memcpy(&length_of_arg, buffer, sizeof(size_t));
        buffer += sizeof(size_t);

        arguments[i] = (char*)malloc(length_of_arg + 1);
        if(!arguments[i])
            print_error_and_die("jobExecutorServer : Error allocating memory for arguments[%d]", i);
        
        memcpy(arguments[i], buffer, length_of_arg);
        arguments[i][length_of_arg] = '\0';
        buffer += length_of_arg;

        length_of_whole_command += length_of_arg + 1;
    }

    // NULL terminate the arguments array
    arguments[number_of_args] = NULL;

    // Construct the whole command
    command = (char*)malloc(length_of_whole_command);
    if(!command)
        print_error_and_die("jobExecutorServer : Error allocating memory for command");
    
    char* temp = command;
    for(int i = 0; i < number_of_args; i++){
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

}

/*
    Issue a job to the buffer and respond to the client.
*/
void issueJob(void* buffer, int client_sock, uint32_t length_of_message, uint32_t number_of_args){
    // Create a new job for the List-Buffer
    Job job = (Job)malloc(sizeof(*job));

    // Job ID
    uint64_t digits = find_digits(id);
    job->jobid = (char*)malloc(digits + 5);
    sprintf(job->jobid, "job_%d", id);
    job->jobid[digits + 4] = '\0';
    id++;

    // Number of arguments and file descriptor
    job->number_of_args = number_of_args;
    job->fd = client_sock;

    // Arguments array and whole command
    constructArgsArray(buffer, job->number_of_args, job->arguments, job->command);

    // Append the job to the buffer. If the buffer is full, wait with condition variable
    pthread_mutex_lock(&buffer_mutex);
    while(isFull(buffer_with_tasks))
        pthread_cond_wait(&buffer_not_full, &buffer_mutex);
    
    appendJob(buffer_with_tasks, job);
    pthread_cond_signal(&buffer_not_empty);
    pthread_mutex_unlock(&buffer_mutex);

    // Send response : JOB <jobID, job> SUBMITTED
    uint32_t length_of_response = 18 + strlen(job->jobid) + strlen(job->command);
    void* response_start = malloc(length_of_response + sizeof(uint32_t));
    if(!response_start)
        print_error_and_die("jobExecutorServer : Error allocating memory for response");

    void* response = response_start;
    memcpy(response, &length_of_response, sizeof(uint32_t));
    response += sizeof(uint32_t);

    // Construct the message
    char* message = malloc(length_of_response + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for message");

    sprintf(message, "JOB <%s, %s> SUBMITTED", job->jobid, job->command);
    message[length_of_response] = '\0';
    memcpy(response, message, length_of_response);

    if(m_write(client_sock, response_start, length_of_response + sizeof(uint32_t)) == -1)
        print_error_and_die("jobExecutorServer : Error writing response to client");
    
    free(message);
    free(response_start);
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
    // Find the number of jobs in the buffer and lock the mutex
    // Unlock it at the end because we don't anyone to change the buffer
    // while we are working with it
    pthread_mutex_lock(&buffer_mutex);
    uint32_t number_of_jobs = buffer_with_tasks->size;

    void* response_start = NULL;
    if(number_of_jobs == 0){
        // Here we can release the mutex because we are not going to change the buffer
        pthread_mutex_unlock(&buffer_mutex);

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

        if(m_write(client_sock, response_start, length_of_message + sizeof(uint32_t)) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");
        

        pthread_mutex_unlock(&buffer_mutex);
    }

    free(response_start);
}

/*
    Exit the server and respond to the client. Server will 
    terminate after all jobs that are running are finished.
    It will remove the jobs from the buffer and send a response
    for the clients waiting : SERVER TERMINATED BEFORE EXECUTION 
*/
void exitServer(int client_sock){
    // Send response : SERVER TERMINATED
    // For the clients that their job did not start send : SERVER TERMINATED BEFORE EXECUTION
    // For the jobs that are running, wait for them to finish

    pthread_mutex_lock(&terminate_mutex);
    terminate = true;
    pthread_mutex_unlock(&terminate_mutex);

    uint32_t length_of_message = 17;
    void* response_start = malloc(length_of_message + sizeof(uint32_t));
    if(!response_start)
        print_error_and_die("jobExecutorServer : Error allocating memory for response");
    
    void* response = response_start;
    memcpy(response, &length_of_message, sizeof(uint32_t));
    response += sizeof(uint32_t);

    char* message = malloc(length_of_message + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for message");
    
    sprintf(message, "SERVER TERMINATED");
    message[17] = '\0';
    memcpy(response, message, length_of_message);

    if(m_write(client_sock, response_start, length_of_message + sizeof(uint32_t)) == -1)
        print_error_and_die("jobExecutorServer : Error writing response to client");
    
    free(message);
    free(response_start);

    // Iterate through the buffer, remove the jobs and send response
    pthread_mutex_lock(&buffer_mutex);
    ListNode current = buffer_with_tasks->head;
    length_of_message = 34;
    while(current != NULL){
        response_start = malloc(length_of_message + sizeof(uint32_t));
        if(!response_start)
            print_error_and_die("jobExecutorServer : Error allocating memory for response");
        
        response = response_start;
        memcpy(response, &length_of_message, sizeof(uint32_t));
        response += sizeof(uint32_t);

        message = malloc(length_of_message + 1);
        if(!message)
            print_error_and_die("jobExecutorServer : Error allocating memory for message");
        
        sprintf(message, "SERVER TERMINATED BEFORE EXECUTION");
        message[34] = '\0';
        memcpy(response, message, length_of_message);

        if(m_write(current->job->fd, response_start, length_of_message + sizeof(uint32_t)) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");
        
        free(message);
        free(response_start);

        ListNode temp = current;
        current = current->next;
        removeJob(buffer_with_tasks, temp->job);
    }
    pthread_mutex_unlock(&buffer_mutex);
}


/*
    This function handles controller threads. They read the 
    message from the client and then put it in the buffer.
*/
void* controller_function(void* arg){
    int client_sock = (*(int*)arg);
    free(arg);

    pthread_mutex_lock(&terminate_mutex);
    active_controller_threads++;
    pthread_mutex_unlock(&terminate_mutex);

    void* buffer_start = NULL;
    void* buffer_ptr = buffer_start;
    uint32_t length_of_message;
    uint32_t number_of_args;
    size_t length_of_command;
    char* command = NULL;
    readCommand(client_sock, buffer_start, buffer_ptr, &length_of_message, &number_of_args, &length_of_command, command);

    // Handle each command differently
    if(strcmp(command, "issueJob") == 0){
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

    pthread_mutex_lock(&terminate_mutex);
    active_controller_threads--;
    if(terminate && active_controller_threads == 0)
        pthread_cond_signal(&terminate_cond);
    
    pthread_mutex_unlock(&terminate_mutex);

    return NULL;
}

/*
    The funtion for the Thread Pool worker threads that execute the jobs.
    They take a job from the buffer and make use of fork and exec 
    system calls. After the job is done they send the result to the client.
*/
void* worker_function(void* arg){
    while(1){
        // Check for termination
        pthread_mutex_lock(&terminate_mutex);
        if(terminate){
            pthread_mutex_unlock(&terminate_mutex);
            break;
        }
        pthread_mutex_unlock(&terminate_mutex);

        ListNode listjob = NULL;
        pthread_mutex_lock(&buffer_mutex);
        while(isEmpty(buffer_with_tasks))
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);

        // Check if concurrency level let us handle the job
        if(worker_threads_working >= concurrency)
            pthread_cond_wait(&worker_cond, &buffer_mutex);
        
        worker_threads_working++;
        listjob = removeFirstJob(buffer_with_tasks);
        pthread_cond_signal(&buffer_not_full);
        
        pthread_mutex_unlock(&buffer_mutex);

        // Handle job
        pid_t pid = fork();
        if(pid == 0){\
            // Create file for output
            // filename will be a cause of leaks, let OS handle it
            pid_t child_pid = getpid();
            uint64_t digits = find_digits(child_pid);
            char* filename = (char*)malloc(digits + 13);
            sprintf(filename, "/tmp/%d.output", child_pid);
            filename[digits + 12] = '\0';

            int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);

            // Redirect stdout output to fd
            dup2(fd, 1);
            close(fd);

            // Execute the job
            execvp(listjob->job->arguments[0], listjob->job->arguments);
            print_error_and_die("jobExecutorServer : Error executing job");
        }
        else if(pid > 0){
            waitpid(pid, NULL, 0);

            // Read the output from the file
            uint64_t digits = find_digits(pid);
            char* filename = (char*)malloc(digits + 13);
            sprintf(filename, "/tmp/%d.output", pid);
            filename[digits + 12] = '\0';

            FILE* file = fopen(filename, "r");
            if(!file)
                print_error_and_die("jobExecutorServer : Error opening file %s", filename);

            // Write start message
            size_t len_of_jobid = strlen(listjob->job->jobid);
            char* start_message = (char*)malloc(25 + len_of_jobid); 
            sprintf(start_message, "-----%s output start-----\n", listjob->job->jobid);
            start_message[24 + len_of_jobid] = '\0';
            if(m_write(listjob->job->fd, start_message, 24 + len_of_jobid) == -1)
                print_error_and_die("jobExecutorServer : Error writing response to client");
            
            free(start_message);

            // Send response in chunks
            char buffer[4096];
            ssize_t bytes_read;
            while((bytes_read = fread(buffer, 1, 4096 - sizeof(uint32_t), file))){
                uint32_t length_of_message = htonl(bytes_read);
                void* response_start = malloc(bytes_read + sizeof(uint32_t));
                if(!response_start)
                    print_error_and_die("jobExecutorServer : Error allocating memory for response");
                
                void* response = response_start;
                memcpy(response, &length_of_message, sizeof(uint32_t));
                response += sizeof(uint32_t);

                memcpy(response, buffer, bytes_read);

                if(m_write(listjob->job->fd, response_start, bytes_read + sizeof(uint32_t)) == -1)
                    print_error_and_die("jobExecutorServer : Error writing response to client");
                
                free(response_start);

            }

            // Write end message
            char* end_message = (char*)malloc(23 + len_of_jobid);
            sprintf(end_message, "-----%s output end-----\n", listjob->job->jobid);
            end_message[22 + len_of_jobid] = '\0';
            if(m_write(listjob->job->fd, end_message, 22 + len_of_jobid) == -1)
                print_error_and_die("jobExecutorServer : Error writing response to client");
            
            free(end_message);

            fclose(file);
            remove(filename);
            free(filename);
            // In the end!
            worker_threads_working--;
            pthread_cond_signal(&worker_cond);
        }
        else{
            // Fork failed
            print_error_and_die("jobExecutorServer : Error forking");
        }
    }

    pthread_exit(NULL);
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

/*
    Check for jobs in the buffer after termination.
    If we find any, we send response to the client.
    Else we just free the memory allocated for the buffer,
    mutexes and condition variables.
*/
void termination(){
    pthread_mutex_lock(&buffer_mutex);
    if(buffer_with_tasks->size > 0){
        ListNode current = buffer_with_tasks->head;
        while(current != NULL){
            uint32_t length_of_message = 34;
            void* response_start = malloc(length_of_message + sizeof(uint32_t));
            if(!response_start)
                print_error_and_die("jobExecutorServer : Error allocating memory for response");
            
            void* response = response_start;
            memcpy(response, &length_of_message, sizeof(uint32_t));
            response += sizeof(uint32_t);

            char* message = malloc(length_of_message + 1);
            if(!message)
                print_error_and_die("jobExecutorServer : Error allocating memory for message");
            
            sprintf(message, "SERVER TERMINATED BEFORE EXECUTION");
            message[34] = '\0';
            memcpy(response, message, length_of_message);

            if(m_write(current->job->fd, response_start, length_of_message + sizeof(uint32_t)) == -1)
                print_error_and_die("jobExecutorServer : Error writing response to client");
            
            free(message);
            free(response_start);

            current = current->next;
        }
    }

    cleanList(buffer_with_tasks);
    pthread_mutex_destroy(&buffer_mutex);

    // Destroy mutexes and condition vars
    pthread_mutex_destroy(&terminate_mutex);
    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&terminate_cond);
    pthread_cond_destroy(&buffer_not_empty);
    pthread_cond_destroy(&buffer_not_full);

}