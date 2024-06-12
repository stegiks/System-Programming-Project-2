#ifndef HELP_H
#define HELP_H

#include "defs.h"

uint64_t find_digits(int num);
bool isNumber(const char* str);
void print_error_and_die(const char* msg, ...);
void memsetzero_and_free(void* buffer, size_t size);
ssize_t m_write(int fd, const void* buffer, uint32_t total_length);
ssize_t m_read(int fd, void** buffer);
int m_open(const char* pathname, int flags);
int m_close(int fd); 

#define CHUNKSIZE 1024

#endif /* help.h */