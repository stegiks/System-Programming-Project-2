
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

../bin/jobCommander localhost $1 issueJob touch myFile.txt
ls ./myFile.txt
../bin/jobCommander localhost $1 issueJob rm myFile.txt
ls ./myFile.txt
../bin/jobCommander localhost $1 exit