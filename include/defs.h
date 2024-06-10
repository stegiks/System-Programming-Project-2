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
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>      // Might be needed for pid_t
#include <stdbool.h>
#include <stdarg.h>     // For variadic functions
#include <ctype.h>

#define PERMISSIONS 0666

#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define RESET "\033[0m"

#endif /* defs.h */