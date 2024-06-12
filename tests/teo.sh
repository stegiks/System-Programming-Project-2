# Check if a port number was provided
if [ -z "$1" ]; then
  echo "Usage: $0 <port number>"
  exit 1
fi

# Assign the first argument to the variable args
args="localhost $1"

../bin/jobCommander $args  issueJob ../bin/progDelay 10 &
sleep 1
../bin/jobCommander $args  issueJob ../bin/progDelay 11 &
sleep 1
../bin/jobCommander $args  issueJob ../bin/progDelay 12 &
sleep 1
../bin/jobCommander $args  issueJob ../bin/progDelay 13 &
sleep 1
../bin/jobCommander $args  issueJob ../bin/progDelay 14 &
sleep 1
../bin/jobCommander $args  poll
../bin/jobCommander $args  setConcurrency 2
../bin/jobCommander $args  poll 
../bin/jobCommander $args  exit