#!/bin/bash

# This is not a test script. It is used to run the jobExecutorServer with the given arguments.
if [ $# -ne 3 ]; then
    echo "Usage: $0 <port> <bufferSize> <threadPoolSize>"
    exit 1
fi

../bin/jobExecutorServer $1 $2 $3