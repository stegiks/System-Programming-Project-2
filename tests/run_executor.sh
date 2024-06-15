
if [ $# -ne 3 ]; then
    echo "Usage: $0 <port> <bufferSize> <threadPoolSize>"
    exit 1
fi

jobExecutorServer $1 $2 $3