
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

jobCommander localhost $1 issueJob touch myFile.txt
ls ./myFile.txt
jobCommander localhost $1 issueJob rm myFile.txt
ls ./myFile.txt
jobCommander localhost $1 exit