#include "../include/queue.h"

/*
    Create a queue with a maximum size
*/
Queue createQueue(uint32_t max_size){
    Queue queue = (Queue)malloc(sizeof(struct queue));
    if(!queue)
        print_error_and_die("queue : Error allocating memory for queue");
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->max_size = max_size;

    return queue;
}

/*
    Check if the queue is empty
*/
bool isEmpty(Queue queue){
    return queue->size == 0;
}

/*
    Check if the queue is full
*/
bool isFull(Queue queue){
    return queue->size == queue->max_size;
}

/*
    Insert a job in the queue
*/
void enqueue(Queue queue, Job job){

    QueueNode new_queue_node = (QueueNode)malloc(sizeof(*new_queue_node));
    if(!new_queue_node)
        print_error_and_die("queue : Error allocating memory for new_queue_node");
    
    new_queue_node->job = job;
    new_queue_node->next = NULL;

    // Make previous last node point to the new node
    if(isEmpty(queue)){
        queue->head = new_queue_node;
        queue->tail = new_queue_node;
    }
    else{
        queue->tail->next = new_queue_node;
        queue->tail = new_queue_node;
    }

    queue->size++;

}

/*
    Returns the first QueueNode in the queue
*/
QueueNode dequeueJob(Queue queue){
    if(isEmpty(queue))
        return NULL;
    
    QueueNode first = queue->head;
    queue->head = first->next;
    queue->size--;

    return first;
}

/*
    Remove a job from the queue
*/
void removeJob(Queue queue, char* jobid){
    QueueNode current = queue->head;
    QueueNode previous = NULL;

    while(current != NULL){
        if(strcmp(current->job->jobid, jobid) == 0){
            if(previous == NULL){
                queue->head = current->next;
            }
            else{
                previous->next = current->next;
            }

            if(current == queue->tail)
                queue->tail = previous;
            
            // Free the node and its contents
            free(current->job->jobid);
            free(current->job->command);
            for(uint32_t i = 0; i < current->job->number_of_args; i++){
                free(current->job->arguments[i]);
            }
            free(current->job->arguments);
            free(current->job);
            free(current);
            queue->size--;
            return;
        }

        previous = current;
        current = current->next;
    }
}

/*
    Remove all jobs from the queue
*/
void cleanQueue(Queue queue){
    QueueNode current = queue->head;

    while(current != NULL){
        QueueNode temp = current->next;

        free(current->job->jobid);
        free(current->job->command);
        for(uint32_t i = 0; i < current->job->number_of_args; i++){
            free(current->job->arguments[i]);
        }
        free(current->job->arguments);
        free(current->job);
        free(current);
        current = temp;
    }

    free(queue);
}

/*
    Free a QueueNode
*/
void freeQueueNode(QueueNode node){
    free(node->job->jobid);
    free(node->job->command);
    for(uint32_t i = 0; i < node->job->number_of_args; i++){
        free(node->job->arguments[i]);
    }
    free(node->job->arguments);
    free(node->job);
    free(node);
}