#ifndef LISTJOBS_H
#define LISTJOBS_H

#include "help.h"

typedef struct job* Job;
typedef struct list_node* ListNode;
typedef struct list* List;

struct job{
    int fd;
    char* jobid;
    char* command;
    char** arguments;
    uint32_t number_of_args;
};

struct list_node{
    Job job;
    ListNode next;
};

struct list{
    ListNode head;
    ListNode tail;
    uint32_t size;
    uint32_t max_size;
};

List createList(uint32_t max_size);
bool isEmpty(List list);
bool isFull(List list);
void appendJob(List list, Job job);
void removeJob(List list, char* jobid);
void cleanList(List list);

#endif /* listjobs.h */