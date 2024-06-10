#include "../include/listjobs.h"

/*
    Create a List with a maximum size
*/
List createList(uint32_t max_size){
    List list = (List)malloc(sizeof(struct list));
    if(!list)
        print_error_and_die("listjobs : Error allocating memory for list");
    
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->max_size = max_size;

    return list;
}

/*
    Check if the list is empty
*/
bool isEmpty(List list){
    return list->size == 0;
}

/*
    Check if the list is full
*/
bool isFull(List list){
    return list->size == list->max_size;
}

/*
    Insert a job in the list
*/
void appendJob(List list, Job job){

    ListNode new_list_node = (ListNode)malloc(sizeof(*new_list_node));
    if(!new_list_node)
        print_error_and_die("listjobs : Error allocating memory for new_list_node");
    
    new_list_node->job = job;
    new_list_node->next = NULL;

    // Make previous last node point to the new node
    if(isEmpty(list)){
        list->head = new_list_node;
        list->tail = new_list_node;
    }
    else{
        list->tail->next = new_list_node;
        list->tail = new_list_node;
    }

    list->size++;

}

/*
    Returns the first ListNode in the list
*/
ListNode dequeueJob(List list){
    if(isEmpty(list))
        return NULL;
    
    ListNode first = list->head;
    list->head = first->next;
    list->size--;

    return first;
}

/*
    Remove a job from the list
*/
void removeJob(List list, char* jobid){
    ListNode current = list->head;
    ListNode previous = NULL;

    while(current != NULL){
        if(strcmp(current->job->jobid, jobid) == 0){
            if(previous == NULL){
                list->head = current->next;
            }
            else{
                previous->next = current->next;
            }

            if(current == list->tail)
                list->tail = previous;
            
            // Free the node and its contents
            free(current->job->jobid);
            free(current->job->command);
            for(uint32_t i = 0; i < current->job->number_of_args; i++){
                free(current->job->arguments[i]);
            }
            free(current->job->arguments);
            free(current->job);
            free(current);
            list->size--;
            return;
        }

        previous = current;
        current = current->next;
    }
}

/*
    Remove all jobs from the list
*/
void cleanList(List list){
    ListNode current = list->head;

    while(current != NULL){
        ListNode temp = current->next;

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

    free(list);
}

/*
    Free a ListNode
*/
void freeListNode(ListNode node){
    free(node->job->jobid);
    free(node->job->command);
    for(uint32_t i = 0; i < node->job->number_of_args; i++){
        free(node->job->arguments[i]);
    }
    free(node->job->arguments);
    free(node->job);
    free(node);
}