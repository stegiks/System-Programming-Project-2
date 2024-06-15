I recomment reading from README.md for a more readable format
either from VS Code or a markdown previewer online : 
https://markdownlivepreview.com/

# System Programming Project 2

## Student Information

- __Name__ : Stefanos Gkikas
- __Student ID__ : 1115202100025
  
## General Info

In this assigment we were asked to implement a multi-threaded server __`(jobExecutorServer)`__ and a client __`(jobCommander)`__ program that can communicate with each other with the use of network sockets. The server is responsible for having worker threads take over the execution of jobs and then return the response to the client. The commands that the client can send to the server are the following:



- __`issueJob <job>`__ : Send a job for execution to the server, where the job is a UNIX-like command.
<br>

- __`setConcurrency <N>`__ : Set the maximum number of jobs that can be executed concurrently to N (N > 0).
<br>

- __`stop <jobid>`__ : Stop (remove from buffer) the job with the given jobid.
<br>

- __`poll`__ : Print all the jobs that are in the buffer.
<br>

- __`exit`__ : Terminate the jobExecutorServer.


## Compilation Instructions - Makefile

In order to compile the project, you need to run `make` in the root directory of the project. This will create 3 executables: __`jobExecutorServer`__, __`jobCommander`__ and __`progDelay`__. progDelay is a simple program that takes an integer as argument and sleeps for that many seconds. It is used to simulate the execution of a job that takes some time to complete.

</br>

Other __PHONY__ targets in the Makefile are:

- __`clean`__ : Removes all executables and object files.
<br>

- __`run_executor`__ : Runs the jobExecutor with default arguments __(ARGS_SER := 8000 10 2)__.
<br>

- __`run_commander`__ : Runs the jobCommander executable with default arguments __(ARGS_COM := localhost 8000 issueJob ls)__.

Argumenents can be changed in the Makefile by changing the values of __`ARGS_SER`__ and __`ARGS_COM`__. 

</br>

You can also run the executables manually by providing the necessary arguments and path. Here is an example of how to run the executables:

```bash
./bin/jobExecutorServer 8000 10 2
./bin/jobCommander localhost 8000 issueJob ls 
```

## Implementation Details

This project consists of 2 main parts, `jobExecutorServer` and `jobCommander`.

### jobExecutorServer

jobExecutorServer has 3 types of threads that are used to handle the incoming connections from the client and execute the jobs that the client sends. These are the files that are used for the implementation of the logic with the threads:

</br>

- [__`jobExecutorServer.c`__](src/jobExecutorServer.c) : The main file of the server. The mutexes and condition variables are initialized here. Also the buffer is created and the server listens for incoming connections from clients with the use of __sockets__. The server creates ThreadPoolSize worker threads that are responsible for executing the jobs coming from the clients (__issueJob__) and before terminating, it waits for all the worker threads to join and controller threads even though they are detached, to ensure that we won't have any memory leaks.

<br>

- [__`serverController.c`__](src/serverController.c) : In this file, we have all the functions that controller threads use to handle the commands that the client sends. For every connection that is accepted by the main thread, a controller thread is created and detached. After reading the command from the socket, the controller thread calls the appropriate function and sends the response back to the client. It then uses [ThreadData struct](include/queue.h) to wait for the worker threads to finish their job and closes the socket that was used for communication with the client. This is done because we don't assume that one response for example from controller thread will be sent before the worker thread finishes its job and so we have to be sure that both threads have finished their job before closing the socket. If the client sends the __`exit`__ command, this controller thread will shut down the socket that the server listens to __(so no more jobs are accepted)__ and main thread will be signaled when the last controller thread finishes its job.
  
<br>

- [__`serverWorker.c`__](src/serverWorker.c) : Worker threads are used to execute UNIX-like commands that the client sends through with the __`issueJob`__ command. The way that the worker threads work is that they wait for a job to be inserted in the buffer from the controller thread. After a worker thread takes a job from the buffer, it uses __fork()__ and __execvp()__ to execute the job. The output is written in a file named __`/tmp/<PID>.output`__, so the parent waits for the child to finish, reads the output from the file and sends it back to the client.
  
    <br>

    > __NOTE__ : jobExecutorServer uses a buffer to store the jobs of type __issueJob__ that the client sends. The buffer is implemented as a __Queue__ and all the necessary functions are in the [__`queue.c`__](src/queue.c) file. The buffer is a __bounded buffer__ and the maximum number of jobs that can be stored in the buffer is set at the beginning of the program with the argument __buffer_size__.
    > 

### jobCommander

jobCommander is the client program that sends commands to the server and receives the response. This executable relies mostly on the __`helpCommander.c`__ file, where all the helper functions are implemented. The main file of the client is [__`jobCommander.c`__](src/jobCommander.c) and the only thing that it does is to call the helper functions.

</br>

- [__`helpCommander.c`__](src/helpCommander.c) : One important choice is the way the client will recieve the response from the server for poll and issueJob commands. For these commands, we must support recieving multiple chunks of data from the server, as the response may be too large to send over the network in one go. To achieve this, together with the bytes of the response, the server sends a number that indicates what kind of chunk is being sent. For `poll` command, type 1 means non-final chunk and type -1 means final chunk. For `issueJob` command, type 1 means non-final chunk of response, type 2 means non-final chunk of output job, type -1 means final chunk of response, type -2 means final chunk of output job and type 0 means termination before sumbitting the job.

    <br>

    > __NOTE__ : When a job is send but not executed, the server sends the message __"SERVER TERMINATED BEFORE EXECUTION"__ in every case except when __stop__ command is the reason for not executing the job. In this case, the server sends the message __""JOB REMOVED BEFORE EXECUTION BY STOP COMMAND"__. In every other case, the responses that are sent back to the client are defined in the assignment description.
    >

### Shared Helper Functions

In the file [__`help.c`__](src/help.c) we have all the functions that are used by both the server and the client. The two key functions that are used everywhere throughout the project are __`m_read`__ and __`m_write`__. These functions are used to read and write from/to the socket and what makes them special is that they use void pointers to write first the size of the whole buffer, some metadata if needed and then the actual message. So, __`m_read`__ is a function that uses malloc inside to return a buffer that contains the message that was sent with the use of __`m_write`__. 

## Communication Protocol and Sychronization

This project uses a client-server model where the connection is established using sockets. The communication protocol used is __`TCP (Transmission Control Protocol)`__, which is a reliable, connection-oriented protocol that ensures the delivery of packets in the same order they were sent.

</br>

For synchronization, the server uses __mutexes and condition variables__ from the __pthread__ library. 

There are three mutexes used:

- __`buffer_mutex`__ : Used to protect the buffer from concurrent access by multiple threads.
<br>

- __`terminate_mutex`__ : This mutex is used to safely check for termination. It ensures safe access to the __terminate__ boolean variable, preventing race conditions
<br>

- __`global_vars_mutex`__ : Used  for safe access to global variables like concurrency and id.

</br>

There are also four condition variables used:

- __`buffer_not_full`__ : Used to make the controller threads wait when the buffer is full.
<br>

- __`buffer_not_empty`__ : Used to make the worker threads wait when the buffer is empty.
<br>

- __`worker_cond`__ : This condition variable is used for checking concurrency. It allows worker threads to check the current level of concurrency and adjust their behavior accordingly.
<br>

- __`controller_cond`__ : This condition variable is used for checking the termination of all controller threads before proceeding with the termination of the server.

</br>


These synchronization techniques ensure that the program operates correctly in a multithreaded environment, preventing race conditions and ensuring that threads can cooperate effectively.

> __NOTE__ : Mutexes and condition variables are used in [ThreadData struct](include/queue.h) also, but they are kind of special as their unique for every __issueJob__ command and their only purpose, as mentioned before, is to make sure that the controller thread waits for the worker thread to finish its job before closing the socket.
> 