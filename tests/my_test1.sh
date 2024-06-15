
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

# Run with 2 Thread Pool
# Without sleeps we can't guarantee the order of the output
# See my_test2.sh for a more controlled test
jobCommander localhost $1 issueJob progDelay 5 &
jobCommander localhost $1 issueJob progDelay 5 &
jobCommander localhost $1 issueJob progDelay 5 &
jobCommander localhost $1 setConcurrency 2
jobCommander localhost $1 issueJob progDelay 5 &
jobCommander localhost $1 issueJob progDelay 5 &
jobCommander localhost $1 issueJob progDelay 5 &
jobCommander localhost $1 stop job_1
jobCommander localhost $1 stop job_3
jobCommander localhost $1 poll
jobCommander localhost $1 issueJob progDelay 1 &
jobCommander localhost $1 exit &