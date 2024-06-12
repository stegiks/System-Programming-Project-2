#ifndef QUEUE_H
#define QUEUE_H

#include "help.h"

typedef struct thread_data* ThreadData;
typedef struct job* Job;
typedef struct queuenode* QueueNode;
typedef struct queue* Queue;

// Needed for determining what thread is 
// going to close the connection with the client
struct thread_data{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool worker_response_ready;
};

struct job{
    int fd;
    char* jobid;
    char* command;
    char** arguments;
    uint32_t number_of_args;
    ThreadData thread_data;
};

struct queuenode{
    Job job;
    QueueNode next;
};

struct queue{
    QueueNode head;
    QueueNode tail;
    uint32_t size;
    uint32_t max_size;
};

Queue createQueue(uint32_t max_size);
bool isEmpty(Queue queue);
bool isFull(Queue queue);
void enqueue(Queue queue, Job job);
QueueNode dequeueJob(Queue queue);
void removeJob(Queue queue, char* jobid);
void cleanQueue(Queue queue);
void freeQueueNode(QueueNode node);

#endif /* queue.h */