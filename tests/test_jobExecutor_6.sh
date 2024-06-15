
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

# Again without sleeps you wont see the expectet output
killall progDelay
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 10 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 10 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 10 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 10 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 10 &
../bin/jobCommander localhost $1 issueJob ../bin/progDelay 10 &
sleep 1
../bin/jobCommander localhost $1 setConcurrency 4
../bin/jobCommander localhost $1 poll
../bin/jobCommander localhost $1 stop job_4
../bin/jobCommander localhost $1 stop job_5
../bin/jobCommander localhost $1 poll
../bin/jobCommander localhost $1 exit