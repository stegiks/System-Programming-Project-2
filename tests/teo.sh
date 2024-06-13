# Check if a port number was provided
if [ -z "$1" ]; then
  echo "Usage: $0 <port number>"
  exit 1
fi

# Assign the first argument to the variable args
args="localhost $1"

../bin/jobCommander $args  issueJob ../bin/progDelay 16 &
sleep 1
../bin/jobCommander $args  issueJob ../bin/progDelay 17 &
sleep 1
../bin/jobCommander $args  issueJob ../bin/progDelay 18 &
sleep 1
../bin/jobCommander $args  issueJob ../bin/progDelay 19 &
sleep 1
../bin/jobCommander $args  issueJob ../bin/progDelay 20 &
sleep 1
../bin/jobCommander $args  poll
../bin/jobCommander $args  setConcurrency 2
../bin/jobCommander $args  poll
../bin/jobCommander $args  stop job_4
../bin/jobCommander $args  poll
../bin/jobCommander $args  exit