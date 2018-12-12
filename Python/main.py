# coding: utf-8
import sys

def main(argv):
	if len(argv) <= 1:
		print 'please input exec .py...'
		return
	test_module = __import__(argv[1], fromlist=[''])
	test_module.run()

if __name__ == '__main__':
	main(sys.argv)