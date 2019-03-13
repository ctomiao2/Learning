# coding: utf-8
from task_lib import *
import socket

def handle_client(client, addr):
	print 'handle_client', client, addr
	while True:
		yield ReadWait(client)
		data = client.recv(65536)
		if not data:
			break
		yield WriteWait(client)
		client.send(data)
	client.close()
	print 'client closed'
	yield

def server(port):
	print 'server starting'
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.bind(('127.0.0.1', port))
	sock.listen(5)
	while True:
		yield ReadWait(sock)
		client, addr = sock.accept()
		yield NewTask(handle_client(client, addr))

def alive():
	while True:
		print 'i am still alive!'
		yield

def run():
	schedule = Scheduler()
	schedule.new(alive())
	schedule.new(server(6666))
	schedule.main_loop()
