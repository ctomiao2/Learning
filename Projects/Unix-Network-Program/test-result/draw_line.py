# -*- coding: utf-8 -*-
import sys
import matplotlib.pyplot as plt

def main():
	print sys.argv
	dir_name = sys.argv[1]
	plt.figure(figsize=(100, 10))
	plt.title("RTT-Line")
	plt.xlabel("segment-sequence")
	plt.ylabel("RTT(ms)")
	plt.yscale('log')
	yticks = []
	yticks.extend(range(0, 100, 10))
	yticks.extend(range(100, 1000, 100))
	yticks.extend(range(1000, 10000, 1000))
	yticks.extend(range(10000, 100000, 10000))
	ylabels = list(str(x) for x in yticks)
	plt.yticks(yticks, ylabels, size='8')

	tcp_snd_ts = {}
	tcp_rcv_ts = {}
	kcp_snd_ts = {}
	kcp_rcv_ts = {}

	confs = [
		(tcp_snd_ts, 'tcp_snd_stats.dat'),
		(tcp_rcv_ts, 'tcp_rcv_stats.dat'),
		(kcp_snd_ts, 'kcp_snd_stats.dat'),
		(kcp_rcv_ts, 'kcp_rcv_stats.dat')
	]

	for conf in confs:
		f = open("%s/%s" % (dir_name, conf[1]), "r")

		while True:
			s = f.readline()
			if not s:
				break
			seq, t = s.split(':')
			conf[0][int(seq)] = int(t)

		f.close()

	# step=10采样
	step, n = 1, 1000
	x, y = [], []
	i = 1
	while i <= n:
		x.append(i)
		if i not in tcp_rcv_ts:
			y.append(100000)
		else:
			y.append(tcp_rcv_ts[i] - tcp_snd_ts[i])
		i += step

	print 'tcp max: %s, min: %s, avg: %s' % (max(y), min(y), float(sum(y))/len(y))

	# tcp折线
	plt.plot(x, y,"r--",linewidth=1)

	x, y = [], []
	i = 1
	while i <= n:
		x.append(i)
		if i not in kcp_rcv_ts:
			y.append(100000)
		else:
			y.append(kcp_rcv_ts[i] - kcp_snd_ts[i])
		i += step

	print 'kcp max: %s, min: %s, avg: %s' % (max(y), min(y), float(sum(y))/len(y))

	# kcp折线
	plt.plot(x, y,"g",linewidth=1)
	plt.show()


if __name__ == '__main__':
	main()