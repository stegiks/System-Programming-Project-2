#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

../bin/jobCommander localhost $1 issueJob ../bin/progDelay 10
../bin/jobCommander localhost $1 poll
../bin/jobCommander localhost $1 exit