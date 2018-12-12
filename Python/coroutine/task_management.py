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
		self.exit_waiting = {}

	def new(self, target):
		task = Task(target)
		self.task_map[task.tid] = task
		self.task_queue.put(task)
		return task.tid

	def wait_for_exit(self, wait_tid, task):
		if wait_tid in self.task_map:
			if self.exit_waiting.get(wait_tid, None) is None:
				self.exit_waiting[wait_tid] = []
			self.exit_waiting[wait_tid].append(task)
			return True
		else:
			# wait_task不存在, 无需waiting
			return False

	def schedule(self, task):
		self.task_queue.put(task)

	def exit(self, task):
		print 'task %d exit!' % task.tid
		del self.task_map[task.tid]
		for task in self.exit_waiting.get(task.tid, []):
			self.schedule(task)


	def main_loop(self):
		while self.task_map:
			task = self.task_queue.get()
			try:
				# r等于生成器函数yield的值
				r = task.run()
				if isinstance(r, SystemCall):
					r.task = task
					r.sched = self
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
		self.sched.schedule(self.task)

class NewTask(SystemCall):
	def __init__(self, target):
		self.target = target

	def handle(self):
		# 创建一个新的task
		tid = self.sched.new(self.target)
		self.task.send_val = tid
		self.sched.schedule(self.task)

class KillTask(SystemCall):
	def __init__(self, tid):
		self.tid = tid

	def handle(self):
		task = self.sched.task_map.get(self.tid, None)
		if task:
			print 'kill task', self.tid
			task.target.close()
			self.task.send_val = True
		else:
			self.task.send_val = False
		self.sched.schedule(self.task)

# wait task tid exit to continue
class WaitTask(SystemCall):
	def __init__(self, tid):
		self.tid = tid

	def handle(self):
		result = self.sched.wait_for_exit(self.tid, self.task)
		# wait_task不存在, 则立即执行
		if not result:
			self.sched.schedule(self.task)

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