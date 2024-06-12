../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost 7000 setConcurrency 2
../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
../bin/jobCommander localhost 7000 stop job_1
../bin/jobCommander localhost 7000 stop job_3
../bin/jobCommander localhost 7000 poll
../bin/jobCommander localhost 7000 issueJob progDelay 1 &

sleep 1
../bin/jobCommander localhost 7000 exit &