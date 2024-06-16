#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

# Run with 3 Thread Pool
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 16 &
sleep 1
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 17 &
sleep 1
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 18 &
sleep 1
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 19 &
sleep 1
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 20 &
sleep 1
../bin/jobCommander localhost $1 poll
../bin/jobCommander localhost $1 setConcurrency 3
../bin/jobCommander localhost $1 poll
../bin/jobCommander localhost $1 stop job_4
../bin/jobCommander localhost $1 poll
../bin/jobCommander localhost $1 exit