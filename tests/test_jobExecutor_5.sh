#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

# Without sleep first poll gets executed 
# faster than some of the issueJob commands
# Also, issueJob commands are not executed in order 
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 10 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 9 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 8 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 7 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 6 &
sleep 1
../bin/jobCommander localhost $1 poll
../bin/jobCommander localhost $1 setConcurrency 2
../bin/jobCommander localhost $1 poll
../bin/jobCommander localhost $1 exit