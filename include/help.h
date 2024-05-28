#ifndef HELP_H
#define HELP_H

#include "defs.h"

void print_error_and_die(const char* msg, ...);
ssize_t m_write(int fd, const void* buffer, uint32_t total_length);

#endif /* help.h */