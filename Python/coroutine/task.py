# coding: utf-8
from task_lib import *

def f1():
	print 'begin f1!!!!!'
	# tid值等于调用f1()的协程的send过来的值
	tid = yield GetTid()
	for i in xrange(10):
		print 'f1: ', tid
		yield

def f2():
	tid = yield GetTid()
	for i in xrange(10):
		print 'f2: ', tid
		yield

def run():
	schedule = Scheduler()
	schedule.new(f1())
	schedule.new(f2())
	schedule.main_loop()