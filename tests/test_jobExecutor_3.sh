
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port>"
    exit 1
fi

jobCommander localhost $1 issueJob ls -l /usr/bin/* /usr/local/bin/* /bin/* /sbin/* /opt/* /etc/* /usr/sbin/*
sleep 1
jobCommander localhost $1 exit