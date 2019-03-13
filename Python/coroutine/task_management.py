# coding: utf-8
from task_lib import *

def get_tid():
	tid = yield GetTid()
	for i in xrange(10):
		print 'test_new_task!!!', tid, i
		yield

def main():
	child = yield NewTask(get_tid())
	print 'waiting for child %s exit!!!' % child
	yield WaitTask(child)
	print 'child %s exit' % child

def run():
	sched = Scheduler()
	sched.new(main())
	sched.main_loop()