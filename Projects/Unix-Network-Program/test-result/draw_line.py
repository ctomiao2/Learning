# -*- coding: utf-8 -*-
import sys
import matplotlib.pyplot as plt

def draw_line(index, title, x1, y1, x2, y2):
	plt.subplot(2, 2, index)
	plt.title(title)
	#plt.xlabel("segment-sequence")
	plt.ylabel("RTT(ms)")
	plt.yscale('log')
	yticks = []
	yticks.extend(range(0, 100, 10))
	yticks.extend(range(100, 1000, 100))
	yticks.extend(range(1000, 10000, 1000))
	yticks.extend(range(10000, 100000, 10000))
	ylabels = list(str(i) for i in yticks)
	plt.yticks(yticks, ylabels, size='8')
	plt.plot(x1, y1, 'r--', linewidth=1)
	plt.plot(x2, y2, 'g--', linewidth=1)

def main():
	print sys.argv
	dir_name = sys.argv[1]
	plt.figure(figsize=(100, 10))

	tcp_snd_ts = {}
	tcp_rcv_ts = {}
	kcp_snd_ts = {}
	kcp_rcv_ts = {}
	rcp1_snd_ts = {}
	rcp1_rcv_ts = {}
	rcp2_snd_ts = {}
	rcp2_rcv_ts = {}

	confs = [
		(tcp_snd_ts, 'tcp_snd_stats.dat', 'tcp', [], []),
		(tcp_rcv_ts, 'tcp_rcv_stats.dat'),
		(kcp_snd_ts, 'kcp_snd_stats.dat', 'kcp', [], []),
		(kcp_rcv_ts, 'kcp_rcv_stats.dat'),
		(rcp1_snd_ts, 'rcp-1_snd_stats.dat', 'rcp-1', [], []),
		(rcp1_rcv_ts, 'rcp-1_rcv_stats.dat'),
		(rcp2_snd_ts, 'rcp-2_snd_stats.dat', 'rcp-2', [], []),
		(rcp2_rcv_ts, 'rcp-2_rcv_stats.dat'),
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

	step, n = 1, 1000
	inst = 0
	while inst < len(confs)/2:
		x, y = [], []
		i = 1
		inst_snd_ts, inst_rcv_ts = confs[2*inst][0], confs[2*inst+1][0]
		while i <= n:
			x.append(i)
			if i not in inst_rcv_ts:
				y.append(100000)
			else:
				y.append(inst_rcv_ts[i] - inst_snd_ts[i])
			i += step

		line_name = confs[2*inst][2]
		print '%s max: %s, min: %s, avg: %s' % (line_name, max(y), min(y), float(sum(y))/len(y))

		confs[2*inst][3].extend(x)
		confs[2*inst][4].extend(y)

		inst += 1

	params = dir_name.split('-')
	params[0], params[1], params[2] = int(params[0]), int(params[1]), int(params[2])
	if params[0] > 0 or params[1] > 0:
		params_str = 'Delay %dms~%dms' % (max(0, params[0]-params[1]), params[0]+params[1])
	elif params[2] > 0:
		params_str = 'Loss %d%%' % params[2]
	else:
		params_str = ''

	# tcp vs kcp
	draw_line(1, 'tcp vs kcp(%s)' % params_str, confs[0][3], confs[0][4], confs[2][3], confs[2][4])
	# kcp vs rcp-1
	draw_line(2, 'kcp vs rcp-1(%s)' % params_str, confs[2][3], confs[2][4], confs[4][3], confs[4][4])
	# kcp vs rcp-2
	draw_line(3, 'kcp vs rcp-2(%s)' % params_str, confs[2][3], confs[2][4], confs[6][3], confs[6][4])
	# rcp-1 vs rcp-2
	draw_line(4, 'rcp-1 vs rcp-2(%s)' % params_str, confs[4][3], confs[4][4], confs[6][3], confs[6][4])

	plt.show()


if __name__ == '__main__':
	main()