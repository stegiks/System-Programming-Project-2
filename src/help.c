#include "../include/help.h"

/*
    This function finds the number of digits in a number
*/
uint64_t find_digits(int num){
    uint64_t digits = 0;
    do {
        num /= 10;
        ++digits;
    } while (num != 0);

    return digits;
}

/*
    This functions checks if a string is a number.
*/
bool isNumber(const char *str){
    for(size_t i = 0; i < strlen(str); i++)
        if(!isdigit(str[i]))
            return false;
        
    return true;
}

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
    uint32_t bytes_left = total_length - sizeof(uint32_t);
    ssize_t n;

    if(write(fd, &total_length, sizeof(uint32_t)) == -1)
        return -1;

    printf("m_write: Just wrote the total length of %d\n", total_length);

    while(bytes_written < total_length){
        n = write(fd, buffer + bytes_written, bytes_left);
        printf("m_write: Wrote another %ld bytes\n", n);
        if(n == -1){
            if(errno == EINTR){
                // write interrupted by signal, retry
                continue;
            }
            else{
                return -1;
            }
        }
        else if(n == 0){
            break;  // EOF reached
        }
        
        bytes_written += n;
        bytes_left -= n;
    }

    return bytes_written;
}

/*
    This function reads from a file descriptor,
    continuously if needed. First read 4 bytes that
    represent the length of the message and then read
    the actual message.
*/
ssize_t m_read(int fd, void** buffer){
    uint32_t total_length;
    ssize_t n;

    // First read the length of the message
    while((n = read(fd, &total_length, sizeof(uint32_t))) == -1)
        if(errno != EINTR)
            return -1;
    
    printf("m_read : Total length is = %d\n", total_length);

    // Allocate memory for the message
    *buffer = malloc(total_length);
    if(!(*buffer))
        return -1;
    
    // Write the length of the message to the buffer
    memcpy(*buffer, &total_length, sizeof(uint32_t));
    
    // Then read the actual message
    uint32_t bytes_read = sizeof(uint32_t);
    uint32_t bytes_left = total_length - sizeof(uint32_t);
    while(bytes_read < total_length){
        n = read(fd, (*buffer) + bytes_read, bytes_left);
        printf("m_read : Read another %ld bytes\n", n);
        if(n == -1){
            if(errno == EINTR){
                // read interrupted by signal, retry
                continue;
            }
            else{
                return -1;
            }
        }
        else if(n == 0){
            break;  // EOF reached
        }

        bytes_read += n;
        bytes_left -= n;
    }

    return bytes_read;
}

/*
    This function closes a file descriptor
*/
int m_close(int fd){
    int ret;
    while((ret = close(fd)) == -1)
        if(errno != EINTR)
            return -1;
    
    return ret;
}

    