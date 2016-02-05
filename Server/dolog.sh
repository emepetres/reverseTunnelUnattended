DATE=$(date +"%d/%m/%Y %H:%M:%S")
PID=$(ps -p $PPID -o ppid=)
PORT=$(netstat -putan | grep tcp | grep "$PID/sshd" | awk -F":" '{print $2}' | awk '{print $1}')
if [ $# == 2 ]; then
	echo -e "$DATE: $1 ($2), en puerto $PORT" >> $1-$2.log
else
	echo -e "$DATE: $1 ($2), en puerto $PORT:\n$3\n" >> $1-$2.log
fi
