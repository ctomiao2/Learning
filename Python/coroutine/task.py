# coding: utf-8
import Queue

class Task(object):
	task_id = 0
	def __init__(self, target):
		Task.task_id += 1
		self.tid = Task.task_id
		self.target = target
		self.send_val = None

	def run(self):
		return self.target.send(self.send_val)

class Scheduler(object):

	def __init__(self):
		self.task_queue = Queue.Queue()
		self.task_map = {}

	def new(self, target):
		task = Task(target)
		self.task_map[task.tid] = task
		self.task_queue.put(task)
		return task.tid

	def schedule(self, task):
		self.task_queue.put(task)

	def exit(self, task):
		print 'task: %d exit!' % task.tid
		del self.task_map[task.tid]

	def main_loop(self):
		while self.task_map:
			task = self.task_queue.get()
			try:
				# r等于生成器函数yield的值
				r = task.run()
				if isinstance(r, SystemCall):
					r.task = task
					r.sche = self
					r.handle()
					continue
			except StopIteration:
				self.exit(task)
				continue
			self.schedule(task)

class SystemCall(object):
	def handle(self):
		pass

class GetTid(SystemCall):
	def handle(self):
		self.task.send_val = self.task.tid
		self.sche.schedule(self.task)

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