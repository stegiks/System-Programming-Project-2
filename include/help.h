#ifndef HELP_H
#define HELP_H

#include "defs.h"

bool isNumber(const char* str);
void print_error_and_die(const char* msg, ...);
ssize_t m_write(int fd, const void* buffer, uint32_t total_length);
ssize_t m_read(int fd, void* buffer);
int m_close(int fd); 

#endif /* help.h */