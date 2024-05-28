#include "../include/help.h"

/*
    This function prints an error message and exits 
    the program. Also prints errno if available
*/
void print_error_and_die(const char* msg, ...){
    va_list args;
    va_start(args, msg);

    // Print the error message
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");

    if(errno != 0)
        fprintf(stderr, "Error : %s\n", strerror(errno));
    
    va_end(args);
    exit(1);
}

/*
    This function writes to a file descriptor
    the given buffer with the given length
*/
ssize_t m_write(int fd, const void* buffer, uint32_t total_length){
    uint32_t bytes_written = 0;
    uint32_t bytes_left = total_length;
    ssize_t n;

    while(bytes_written < total_length){
        n = write(fd, buffer + bytes_written, bytes_left);
        if(n == -1){
            if(errno == EINTR){
                // write interrupted by signal, retry
                continue;
            }
            else{
                return -1;
            }
        }
        if(n == 0){
            break;  // EOF reached
        }
        
        bytes_written += n;
        bytes_left -= n;
    }

    return bytes_written;
}

    