#include "../include/helpserver.h"

uint32_t id = 1;
int concurrency = 1;

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

    printf("Controller %ld : length of message is %d\n", pthread_self(), *length_of_message);

    memcpy(number_of_args, *buffer_ptr, sizeof(uint32_t));
    *buffer_ptr += sizeof(uint32_t);

    memcpy(length_of_command, *buffer_ptr, sizeof(size_t));
    *command = (char*)malloc(*length_of_command + 1);
    if(!(*command))
        print_error_and_die("jobExecutorServer : Error allocating memory for command");
    
    memcpy(*command, (*buffer_ptr) + sizeof(size_t), *length_of_command);
    (*command)[*length_of_command] = '\0';
}

/*
    Construct the arguments array for the job.
*/
char** constructArgsArray(void* buffer, uint32_t number_of_args, char** command){
    char** arguments = (char**)malloc((number_of_args + 1) * sizeof(char*));
    if(!arguments)
        print_error_and_die("jobExecutorServer : Error allocating memory for arguments");
    
    uint32_t length_of_whole_command;
    // Arguments array
    for(uint32_t i = 0; i < number_of_args; i++){
        size_t length_of_arg;
        memcpy(&length_of_arg, buffer, sizeof(size_t));
        buffer += sizeof(size_t);
        // printf("Size of arg : %ld\n", length_of_arg);

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
    // Create a new job for the List-Buffer
    Job job = (Job)malloc(sizeof(*job));

    // Job ID
    pthread_mutex_lock(&global_vars_mutex);
    printf("Controller %ld: Locked global_vars_mutex\n", pthread_self());
    uint64_t digits = find_digits(id);
    job->jobid = (char*)malloc(digits + 5);
    sprintf(job->jobid, "job_%d", id);
    job->jobid[digits + 4] = '\0';

    id++;
    printf("Controller %ld: Unlocking global_vars_mutex\n", pthread_self());
    pthread_mutex_unlock(&global_vars_mutex);

    // Number of arguments and file descriptor
    job->number_of_args = number_of_args;
    job->fd = client_sock;

    // Thread data. Make it point to the same address as the argument
    job->thread_data = m_thread_data;

    // Arguments array and whole command
    job->arguments = constructArgsArray(buffer, job->number_of_args, &(job->command));

    // Append the job to the buffer. If the buffer is full, wait with condition variable
    pthread_mutex_lock(&buffer_mutex);
    printf("Controller %ld : Locked buffer_mutex\n", pthread_self());
    while(isFull(buffer_with_tasks))
        pthread_cond_wait(&buffer_not_full, &buffer_mutex);
    
    appendJob(buffer_with_tasks, job);
    pthread_cond_signal(&buffer_not_empty);
    printf("Controller %ld : Unlocking buffer_mutex\n", pthread_self());
    pthread_mutex_unlock(&buffer_mutex);

    // Send response : JOB <jobID, job> SUBMITTED
    uint32_t length_of_response = 18 + strlen(job->jobid) + strlen(job->command);
    printf("Controller %ld : Length of response : %d\n", pthread_self(), length_of_response);

    // Construct the message
    char* message = malloc(length_of_response + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for message");

    sprintf(message, "JOB <%s, %s> SUBMITTED", job->jobid, job->command);
    message[length_of_response] = '\0';

    // We need to write the response in chunks just like in the worker function
    // Write an integer at the start to indicate that it is a response
    // 1 for response, 2 for output, -1 for end of response, -2 for end of output
    uint32_t bytes_written = 0;
    uint32_t bytes_left = length_of_response;
    while(bytes_written < length_of_response){
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

        void* response = malloc(size_to_write + sizeof(int));
        if(!response)
            print_error_and_die("jobExecutorServer : Error allocating memory for response");
        
        memcpy(response, &type, sizeof(int));
        memcpy(response + sizeof(int), message + bytes_written, size_to_write);

        uint32_t total_size = size_to_write + sizeof(int) + sizeof(uint32_t);
        printf("Controller %ld : Total size is %d\n", pthread_self(), total_size);
        if(m_write(client_sock, response, total_size) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");
        
        bytes_written += size_to_write;
        bytes_left -= size_to_write;

        free(response);
    }
    
    printf("Controller %ld : Response written to client\n", pthread_self());
    free(message);
}


/*
    Set the concurrency of the server and respond to the client.
*/
void setConcurrency(void* buffer, int client_sock){
    size_t length_of_setconcurrency;
    memcpy(&length_of_setconcurrency, buffer, sizeof(size_t));
    buffer += sizeof(size_t);
    
    // Get pass string of setConcurrency
    buffer += length_of_setconcurrency;

    // Get the length of the new concurrency
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
    printf("Controller %ld : The string of new concurrency is %s\n", pthread_self(), new_concurrency);
    printf("Controller %ld : Concurrency set to %d\n", pthread_self(), concurrency);

    // Signal the worker threads to check the new concurrency
    // ! BROADCAST BETTER TO 
    int threads_to_signal = concurrency - old_concurrency;
    if(threads_to_signal > 0){
        pthread_mutex_lock(&buffer_mutex);
        for(int i = 0; i < threads_to_signal; i++)
            pthread_cond_signal(&worker_cond);
        
        pthread_mutex_unlock(&buffer_mutex);
    }

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
    
    printf("Controller %ld : message = %s\n", pthread_self(), message);
    free(new_concurrency);
    free(message);
}

/*
    Remove a job from the buffer and respond to the client.
*/
void stopJob(void* buffer, int client_sock){
    size_t length_of_stop;
    memcpy(&length_of_stop, buffer, sizeof(size_t));
    buffer += sizeof(size_t);

    // Get pass string of stop
    buffer += length_of_stop;

    size_t length_of_jobid;
    memcpy(&length_of_jobid, buffer, sizeof(size_t));
    buffer += sizeof(size_t);

    char* jobid = (char*)malloc(length_of_jobid + 1);
    if(!jobid)
        print_error_and_die("jobExecutorServer : Error allocating memory for jobid");
    
    memcpy(jobid, buffer, length_of_jobid);
    jobid[length_of_jobid] = '\0';

    printf("Controller %ld : Searching for a jobid with value = %s\n", pthread_self(), jobid);

    // Find the job in the shared buffer, so use the mutex for safety
    pthread_mutex_lock(&buffer_mutex);
    ListNode current = buffer_with_tasks->head;
    bool found = false;
    while(current != NULL){
        if(strcmp(current->job->jobid, jobid) == 0){

            // Send client : JOB REMOVED BEFORE EXECUTION BY STOP COMMAND
            int type = -2;
            char message[45] = "JOB REMOVED BEFORE EXECUTION BY STOP COMMAND";
            message[44] = '\0';
            void* removed_response = malloc(44 + sizeof(int));
            if(!removed_response)
                print_error_and_die("jobExecutorServer : Error allocating memory for removed_response");
            
            memcpy(removed_response, &type, sizeof(int));
            memcpy(removed_response + sizeof(int), message, 44);

            uint32_t total_size = 44 + sizeof(int) + sizeof(uint32_t);
            printf("Controller %ld : Total size is %d\n", pthread_self(), total_size);
            if(m_write(current->job->fd, removed_response, total_size) == -1)
                print_error_and_die("jobExecutorServer : Error writing response to client");

            // Remove the job from the buffer and signal controller threads
            removeJob(buffer_with_tasks, current->job->jobid);
            pthread_cond_signal(&buffer_not_full);
            found = true;
            break;
        }
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
    // Find the number of jobs in the buffer and lock the mutex
    // Unlock it at the end because we don't anyone to change the buffer
    // while we are working with it
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

        if(m_write(client_sock, (void*)(message), length_of_message + sizeof(uint32_t)) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");
        
        free(message);
    }
    else{
        // Send response : 
        // [number of jobs] [length of jobid1] [jobid1] [length of job1] [job1] ...
        // Send [number of jobs] first
        // Then send [length of jobid] [jobid] [length of job] [job] for each job seperately
        if(write(client_sock, &number_of_jobs, sizeof(uint32_t)) == -1)
            print_error_and_die("jobExecutorServer : Error writing response to client");
        
        ListNode current = buffer_with_tasks->head;
        while(current != NULL){
            size_t length_of_jobid = strlen(current->job->jobid);
            if(m_write(client_sock, current->job->jobid, length_of_jobid + sizeof(size_t)) == -1)
                print_error_and_die("jobExecutorServer : Error writing response to client");
            
            // job command must be written in chunks of CHUNKSIZE
            // because it may be too big. Before each chunk we write
            // an integer to indicate the type of message. 1 for non-final and -1 for final
            uint32_t bytes_written = 0;
            uint32_t bytes_left = strlen(current->job->command);
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

                void* response = malloc(size_to_write + sizeof(int));
                if(!response)
                    print_error_and_die("jobExecutorServer : Error allocating memory for response");
                
                memcpy(response, &type, sizeof(int));
                memcpy(response + sizeof(int), current->job->command + bytes_written, size_to_write);

                uint32_t total_size = size_to_write + sizeof(int) + sizeof(uint32_t);
                printf("Controller %ld : Total size is %d\n", pthread_self(), total_size);
                if(m_write(client_sock, response, total_size)== -1)
                    print_error_and_die("jobExecutorServer : Error writing response to client");
                
                bytes_written += size_to_write;
                bytes_left -= size_to_write;

                free(response);
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
        removeJob(buffer_with_tasks, temp->job->jobid);
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
    printf("Controller %ld : Locked terminate_mutex\n", pthread_self());
    active_controller_threads++;
    printf("Controller %ld : Active controller threads : %d\n", pthread_self(), active_controller_threads);
    pthread_mutex_unlock(&terminate_mutex);

    void* buffer_start = NULL;
    void* buffer_ptr = buffer_start;
    uint32_t length_of_message;
    uint32_t number_of_args;
    size_t length_of_command;
    char* command = NULL;
    printf("Controller %ld : Reading command\n", pthread_self());
    readCommand(client_sock, &buffer_start, &buffer_ptr, &length_of_message, &number_of_args, &length_of_command, &command);
    printf("Controller %ld : Command read\n", pthread_self());

    if(command)
        printf("Controller %ld : Command is %s\n", pthread_self(), command);

    // Handle each command differently
    ThreadData thread_data = NULL;
    if(strcmp(command, "issueJob") == 0){
        thread_data = (ThreadData)malloc(sizeof(*thread_data));
        if(!thread_data)
            print_error_and_die("jobExecutorServer : Error allocating memory for thread_data");
         
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
    printf("Controller %ld : Controller thread finished\n", pthread_self());
    if(thread_data != NULL){
        printf("Controller %ld : Waiting for worker to finish\n", pthread_self());
        pthread_mutex_lock(&(thread_data->mutex));
        while(!(thread_data->worker_response_ready))
            pthread_cond_wait(&(thread_data->cond), &(thread_data->mutex));
        
        pthread_mutex_unlock(&(thread_data->mutex));
        printf("Controller %ld : Freeing thread_data controller\n", pthread_self());
        free(thread_data);
    }

    // Free the memory and close the socket
    if(m_close(client_sock) == -1)
        print_error_and_die("jobExecutorServer : Error closing client socket");
    
    printf("Controller %ld : Client socket closed\n", pthread_self());

    free(command);
    free(buffer_start);

    pthread_mutex_lock(&terminate_mutex);
    printf("Controller %ld : Checking for termination\n", pthread_self());
    active_controller_threads--;
    if(terminate && active_controller_threads == 0)
        pthread_cond_signal(&terminate_cond);
    
    printf("Controller %ld : Termination checked\n", pthread_self());
    pthread_mutex_unlock(&terminate_mutex);

    return NULL;
}

/*
    The funtion for the Thread Pool worker threads that execute the jobs.
    They take a job from the buffer and make use of fork and exec 
    system calls. After the job is done they send the result to the client.
*/
void* worker_function(){
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
        printf("Worker %ld : Locked buffer_mutex worker\n", pthread_self());
        while(isEmpty(buffer_with_tasks)){
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
            
            // Check for termination after possible broadcast 
            pthread_mutex_lock(&terminate_mutex);
            if(terminate){
                pthread_mutex_unlock(&terminate_mutex);
                pthread_mutex_unlock(&buffer_mutex);
                break;
            }
            pthread_mutex_unlock(&terminate_mutex); 
        }


        // Check if concurrency level let us handle the job
        pthread_mutex_lock(&global_vars_mutex);
        printf("Worker %ld : Locked global_vars_mutex worker\n", pthread_self());
        while (worker_threads_working >= concurrency){
            printf("Worker %ld : Worker waiting because of concurrency\n", pthread_self());
            pthread_mutex_unlock(&buffer_mutex);
            pthread_cond_wait(&worker_cond, &global_vars_mutex);
            pthread_mutex_lock(&buffer_mutex);
        }
        
        worker_threads_working++;
        printf("Worker %ld : Worker threads working : %d\n", pthread_self(), worker_threads_working);
        pthread_mutex_unlock(&global_vars_mutex);
        listjob = dequeueJob(buffer_with_tasks);
        if(listjob == NULL){
            printf("Worker %ld : Listjob is NULL\n", pthread_self());
            worker_threads_working--;
            pthread_mutex_unlock(&buffer_mutex);
            continue;
        }
        pthread_cond_signal(&buffer_not_full);
        
        printf("Worker %ld : Unlocked buffer_mutex worker\n", pthread_self());
        pthread_mutex_unlock(&buffer_mutex);

        // Handle job
        pid_t pid = fork();
        if(pid == 0){
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
            execvp(listjob->job->arguments[1], (listjob->job->arguments + 1));
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

            //! Write start message
            size_t len_of_jobid = strlen(listjob->job->jobid);
            char* start_message = (char*)malloc(25 + len_of_jobid); 
            if(!start_message)
                print_error_and_die("jobExecutorServer : Error allocating memory for start message");
            
            sprintf(start_message, "-----%s output start-----\n", listjob->job->jobid);
            start_message[24 + len_of_jobid] = '\0';

            // Select type of message
            int type = 2;
            void* start_message_with_type = malloc(25 + len_of_jobid + sizeof(int));
            if(!start_message_with_type)
                print_error_and_die("jobExecutorServer : Error allocating memory for start message with type response");

            memcpy(start_message_with_type, &type, sizeof(int));
            memcpy(start_message_with_type + sizeof(int), start_message, 24 + len_of_jobid);

            uint32_t total_size = 24 + len_of_jobid + sizeof(int) + sizeof(uint32_t);
            printf("Worker %ld : Total size is %d\n", pthread_self(), total_size);
            if(m_write(listjob->job->fd, start_message_with_type, total_size) == -1)
                print_error_and_die("jobExecutorServer : Error writing response to client");
            
            free(start_message);
            free(start_message_with_type);
            //! End of start message

            //! Send response in chunks
            while(true){
                void* buffer = malloc(CHUNKSIZE + sizeof(int));
                if(!buffer)
                    print_error_and_die("jobExecutorServer : Error allocating memory for buffer");
                
                int type = 2;
                memcpy(buffer, &type, sizeof(int));

                ssize_t n = fread(buffer + sizeof(int), 1, CHUNKSIZE, file);
                if(n <= 0){
                    free(buffer);
                    break;
                }

                // Write the chunk
                uint32_t total_size = n + sizeof(int) + sizeof(uint32_t);
                printf("Worker %ld : Total size is %d\n", pthread_self(), total_size);
                if(m_write(listjob->job->fd, buffer, total_size) == -1)
                    print_error_and_die("jobExecutorServer : Error writing output chunk to client");
                
                memset(buffer, 0, CHUNKSIZE + sizeof(int));
                free(buffer);
            }
            //! End of response

            //! Write end message
            char* end_message = (char*)malloc(23 + len_of_jobid);
            if(!end_message)
                print_error_and_die("jobExecutorServer : Error allocating memory for end message");

            sprintf(end_message, "-----%s output end-----\n", listjob->job->jobid);
            end_message[22 + len_of_jobid] = '\0';

            type = -2;
            void* end_message_with_type = malloc(23 + len_of_jobid + sizeof(int));
            if(!end_message_with_type)
                print_error_and_die("jobExecutorServer : Error allocating memory for end message with type response");
            
            memcpy(end_message_with_type, &type, sizeof(int));
            memcpy(end_message_with_type + sizeof(int), end_message, 22 + len_of_jobid);

            total_size = 22 + len_of_jobid + sizeof(int) + sizeof(uint32_t);
            printf("Worker %ld : Total size is %d\n", pthread_self(), total_size);
            if(m_write(listjob->job->fd, end_message_with_type, total_size) == -1)
                print_error_and_die("jobExecutorServer : Error writing response to client");
            
            free(end_message);
            free(end_message_with_type);
            //! End of end message

            printf("Worker %ld : Output sent\n", pthread_self());

            // Signal the controller thread that the response is ready
            // so that it can close the socket
            pthread_mutex_lock(&(listjob->job->thread_data->mutex));
            listjob->job->thread_data->worker_response_ready = true;
            pthread_cond_signal(&(listjob->job->thread_data->cond));
            pthread_mutex_unlock(&(listjob->job->thread_data->mutex));

            fclose(file);
            remove(filename);
            free(filename);
            // In the end!
            printf("Worker %ld : Locking global_vars_mutex\n", pthread_self());
            pthread_mutex_lock(&global_vars_mutex);
            printf("Worker %ld : Locked global_vars_mutex\n", pthread_self());
            worker_threads_working--;
            printf("Worker %ld : Unlocked global_vars_mutex\n", pthread_self());
            pthread_mutex_unlock(&global_vars_mutex);
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