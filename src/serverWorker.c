#include "../include/helpserver.h"

/*
    Funtion that sends the fixed messages
*/
void send_fixed_message(int fd, int type, const char* jobid_str, size_t length_of_jobid){
    // Allocate memory for the message
    uint32_t fixed_message_size = (type == 2) ? 24 : 22;
    char* message = (char*)malloc(fixed_message_size + length_of_jobid + 1);
    if(!message)
        print_error_and_die("jobExecutorServer : Error allocating memory for fixed message");
    
    if(type == 2)
        sprintf(message, "-----%s output start-----\n", jobid_str);
    else
        sprintf(message, "-----%s output end-----\n", jobid_str);
    
    message[fixed_message_size + length_of_jobid] = '\0';

    // Write the type as well
    void* message_with_type = malloc(fixed_message_size + length_of_jobid + sizeof(int) + 1);
    if(!message_with_type)
        print_error_and_die("jobExecutorServer : Error allocating memory for fixed message with type response");
    
    memcpy(message_with_type, &type, sizeof(int));
    memcpy(message_with_type + sizeof(int), message, fixed_message_size + length_of_jobid);

    uint32_t total_size = fixed_message_size + length_of_jobid + sizeof(int) + sizeof(uint32_t);
    if(m_write(fd, message_with_type, total_size) == -1)
        print_error_and_die("jobExecutorServer : Error writing response to client");
    
    free(message);
    free(message_with_type);
}

/*
    This is the funtion for worker threads that execute the jobs.
    They take a job from the buffer and make use of fork and exec 
    system calls. After the job is done they send the result to the client.
*/
void* worker_function(){

    while(1){
        // Check for termination
        pthread_mutex_lock(&terminate_mutex);
        if(terminate){
            pthread_mutex_unlock(&terminate_mutex);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&terminate_mutex);

        // Check if there are jobs in the buffer. If not wait
        pthread_mutex_lock(&buffer_mutex);
        while(isEmpty(buffer_with_tasks)){
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
            
            // Check for termination after possible broadcast from exitServer()
            pthread_mutex_lock(&terminate_mutex);
            if(terminate){
                pthread_mutex_unlock(&terminate_mutex);
                pthread_mutex_unlock(&buffer_mutex);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&terminate_mutex); 
        }
        pthread_mutex_unlock(&buffer_mutex);

        // Check if concurrency level let us handle the job. If not wait
        pthread_mutex_lock(&global_vars_mutex);
        while (worker_threads_working >= concurrency){
            pthread_cond_wait(&worker_cond, &global_vars_mutex);

            // Check for termination after possible broadcast from exitServer()
            pthread_mutex_lock(&terminate_mutex);
            if(terminate){
                pthread_mutex_unlock(&terminate_mutex);
                pthread_mutex_unlock(&global_vars_mutex);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&terminate_mutex);
        }
        
        worker_threads_working++;
        pthread_mutex_unlock(&global_vars_mutex);

        // Dequeue the job after passing the above checks
        pthread_mutex_lock(&buffer_mutex);
        QueueNode queuejob = dequeueJob(buffer_with_tasks);
        if(queuejob == NULL){
            // Some other worker thread that was previously "working" have taken the job
            pthread_mutex_lock(&global_vars_mutex);
            worker_threads_working--;
            pthread_cond_signal(&worker_cond);
            pthread_mutex_unlock(&global_vars_mutex);
            pthread_mutex_unlock(&buffer_mutex);
            continue;
        }
        // Signal the controller thread that more space is available in the buffer
        pthread_cond_signal(&buffer_not_full);
        pthread_mutex_unlock(&buffer_mutex);

        // Handle job
        pid_t pid = fork();
        if(pid == 0){

            // Create file for output
            char filename[100];
            pid_t child_pid = getpid();
            uint64_t digits = find_digits(child_pid);
            sprintf(filename, "/tmp/%d.output", child_pid);
            filename[digits + 12] = '\0';

            // Redirect stdout output to fd
            int fd;
            if((fd = m_open(filename, O_WRONLY | O_CREAT | O_TRUNC)) == -1){
                perror("jobExecutorServer : Error opening file");
                _exit(1);
            }

            if(dup2(fd, STDOUT_FILENO) == -1){
                perror("jobExecutorServer : Error duplicating file descriptor");
                _exit(1);
            }

            if(m_close(fd) == -1){
                perror("jobExecutorServer : Error closing file descriptor");
                _exit(1);
            }

            // Execute the job
            execvp(queuejob->job->arguments[0], queuejob->job->arguments);

            // Use _exit to avoid cleanups that may affect the parent
            perror("jobExecutorServer : Error executing job");
            _exit(1);
        }
        else if(pid > 0){
            waitpid(pid, NULL, 0);

            // Create filename to read from
            uint64_t digits = find_digits(pid);
            char* filename = (char*)malloc(digits + 13);
            sprintf(filename, "/tmp/%d.output", pid);
            filename[digits + 12] = '\0';

            // Use open to get the file descriptor
            int fd;
            if((fd = open(filename, O_RDONLY)) == -1)
                print_error_and_die("jobExecutorServer : Error opening file %s", filename);

            // Write start message : "-----jobid output start-----\n"
            size_t len_of_jobid = strlen(queuejob->job->jobid);
            send_fixed_message(queuejob->job->fd, 2, queuejob->job->jobid, len_of_jobid);

            // Send output of the job in chunks
            while(true){
                void* buffer = malloc(CHUNKSIZE + sizeof(int));
                if(!buffer)
                    print_error_and_die("jobExecutorServer : Error allocating memory for buffer");
                
                int type = 2;
                memcpy(buffer, &type, sizeof(int));

                ssize_t n;
                if((n = read(fd, buffer + sizeof(int), CHUNKSIZE)) == -1)
                    print_error_and_die("jobExecutorServer : Error reading from file");

                if(n <= 0){
                    free(buffer);
                    break;
                }

                // Write the chunk
                uint32_t total_size = n + sizeof(int) + sizeof(uint32_t);
                if(m_write(queuejob->job->fd, buffer, total_size) == -1)
                    print_error_and_die("jobExecutorServer : Error writing output chunk to client");
                
                memsetzero_and_free(buffer, CHUNKSIZE + sizeof(int));
            }

            // Write end message : "-----jobid output end-----\n"
            send_fixed_message(queuejob->job->fd, -2, queuejob->job->jobid, len_of_jobid);

            // Signal the controller thread that the response is ready
            // so that it can close the socket
            pthread_mutex_lock(&(queuejob->job->thread_data->mutex));
            queuejob->job->thread_data->worker_response_ready = true;
            pthread_cond_signal(&(queuejob->job->thread_data->cond));
            pthread_mutex_unlock(&(queuejob->job->thread_data->mutex));

            // Free the job and the queue node
            freeQueueNode(queuejob);

            if(m_close(fd) == -1)
                print_error_and_die("jobExecutorServer : Error closing file %s", filename);

            remove(filename);
            free(filename);

            // Signal working threads waiting on concurrency check
            pthread_mutex_lock(&global_vars_mutex);
            worker_threads_working--;
            pthread_cond_signal(&worker_cond);
            pthread_mutex_unlock(&global_vars_mutex);
        }
        else{
            // Fork failed
            print_error_and_die("jobExecutorServer : Error forking");
        }
    }
}