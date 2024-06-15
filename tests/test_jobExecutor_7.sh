
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

killall progDelay
jobCommander localhost $1 issueJob progDelay 10 &
jobCommander localhost $1 stop job_2
jobCommander localhost $1 exit