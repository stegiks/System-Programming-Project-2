
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

jobCommander localhost $1 issueJob progDelay 20
jobCommander localhost $1 poll
jobCommander localhost $1 exit