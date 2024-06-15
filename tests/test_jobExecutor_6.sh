
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

# Again without sleeps you wont see the expectet output
killall progDelay
jobCommander localhost $1 issueJob progDelay 10 &
jobCommander localhost $1 issueJob progDelay 10 &
jobCommander localhost $1 issueJob progDelay 10 &
jobCommander localhost $1 issueJob progDelay 10 &
jobCommander localhost $1 issueJob progDelay 10 &
jobCommander localhost $1 issueJob progDelay 10 &
# sleep 1
jobCommander localhost $1 setConcurrency 4
jobCommander localhost $1 poll
jobCommander localhost $1 stop job_4
jobCommander localhost $1 stop job_5
jobCommander localhost $1 poll
jobCommander localhost $1 exit