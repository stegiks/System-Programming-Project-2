if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

# Run with 3 Thread Pool
jobCommander localhost $1 issueJob progDelay 16 &
sleep 1
jobCommander localhost $1 issueJob progDelay 17 &
sleep 1
jobCommander localhost $1 issueJob progDelay 18 &
sleep 1
jobCommander localhost $1 issueJob progDelay 19 &
sleep 1
jobCommander localhost $1 issueJob progDelay 20 &
sleep 1
jobCommander localhost $1 poll
jobCommander localhost $1 setConcurrency 3
jobCommander localhost $1 poll
jobCommander localhost $1 stop job_4
jobCommander localhost $1 poll
jobCommander localhost $1 exit