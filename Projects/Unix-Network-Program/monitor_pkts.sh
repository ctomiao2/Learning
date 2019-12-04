if [ $1 == 1 ]
then
    sudo iptables -A INPUT -p tcp --dport 7000
    sudo iptables -A INPUT -p udp --dport 7000
    sudo iptables -A OUTPUT -p tcp --sport 7000
    sudo iptables -A OUTPUT -p udp --sport 7000
    echo "add monitor for tcp and udp: 7000"
elif [ $1 == 0 ]
then
    sudo iptables -D INPUT -p tcp --dport 7000
    sudo iptables -D INPUT -p udp --dport 7000
    sudo iptables -D OUTPUT -p tcp --sport 7000
    sudo iptables -D OUTPUT -p udp --sport 7000
    echo "del monitor for tcp and udp: 7000"
elif [ $1 == 2 ]
then
    sudo iptables -Z INPUT
    sudo iptables -Z OUTPUT
    echo "reset monitor"
else
    sudo iptables -L -v -n -x
fi
