valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 setConcurrency 2
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 issueJob ../bin/progDelay 5 &
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 stop job_1
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 stop job_3
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 poll
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 issueJob progDelay 1 &

# sleep 1
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ../bin/jobCommander localhost 7000 exit &