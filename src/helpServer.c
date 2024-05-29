#include "../include/helpserver.h"

uint32_t id = 1;
uint32_t concurrency = 1;

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
    This function is used to validate the arguments given in the command line.
*/
bool validate_arguments(int argc, char** argv){
    if(argc != 4)
        return false;
    
    if(!isNumber(argv[1]) || !isNumber(argv[2]) || !isNumber(argv[3]))
        return false;
    
    int port = atoi(argv[1]);
    int buffersize = atoi(argv[2]);
    int threadPoolSize = atoi(argv[3]);

    // Check for valid numbers
    if(port < 0 || port > 65535 || buffersize < 1 || threadPoolSize < 1)
        return false;

    return true;
}