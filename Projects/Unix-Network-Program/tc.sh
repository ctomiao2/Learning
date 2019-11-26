sudo tc qdisc del dev eth0 root
sudo tc qdisc add dev eth0 root handle 1: prio priomap 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
sudo tc qdisc add dev eth0 parent 1:2 handle 20: netem delay $1ms $2ms $3% loss $4% 
sudo tc filter add dev eth0 parent 1:0 protocol ip u32 match ip sport 7000 0xffff flowid 1:2
tc -s qdisc ls dev eth0
