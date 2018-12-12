# coding: utf-8
import time

def tail(logfile, target):
    logfile.seek(0, 2)
    while True:
        line = logfile.readline()
        if not line:
            time.sleep(0.1)
            continue
        target.send(line)

def coroutine(func):
    def wrap_func(*args, **kwargs):
        r = func(*args, **kwargs)
        r.next()
        return r
    return wrap_func

@coroutine
def grep(pattern, target):
    while True:
        line = (yield)
        if pattern in line:
            target.send(line)
           
@coroutine
def printer():
    while True:
        line = (yield)
        print line

def run():
	f = open('coroutine/logtail.txt')
	tail(f, grep('1', printer()))