
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

# Without sleep first poll gets executed 
# faster than some of the issueJob commands
# Also, issueJob commands are not executed in order 
jobCommander localhost $1 issueJob progDelay 10 &
jobCommander localhost $1 issueJob progDelay 9 &
jobCommander localhost $1 issueJob progDelay 8 &
jobCommander localhost $1 issueJob progDelay 7 &
jobCommander localhost $1 issueJob progDelay 6 &
# sleep 1
jobCommander localhost $1 poll
jobCommander localhost $1 setConcurrency 2
jobCommander localhost $1 poll
jobCommander localhost $1 exit