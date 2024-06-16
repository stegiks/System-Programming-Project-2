#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

../bin/jobCommander localhost $1 issueJob ../bin/progDelay 2
../bin/jobCommander localhost $1 exit