
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

# Run with 2 Thread Pool
# Without sleeps we can't guarantee the order of the output
# See my_test2.sh for a more controlled test
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost $1 setConcurrency 2
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost $1 stop job_1
../bin/jobCommander localhost $1 stop job_3
../bin/jobCommander localhost $1 poll
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 1 &
../bin/jobCommander localhost $1 exit &