#ifndef DEFS_H
#define DEFS_H

// Useful definitions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>      // Might be needed for pid_t

typedef enum boolean{
    false,
    true
} bool;

#endif /* defs.h */