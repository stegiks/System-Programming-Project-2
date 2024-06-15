
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

# This will fail : issueJob poll is not a valid command
killall progDelay
jobCommander localhost $1 issueJob poll
jobCommander localhost $1 exit