import subprocess
import sys

def test_single(size):
	out = subprocess.check_output(["./mem",str(size)])
	lines = out.splitlines()
	print lines[2]
	l = map(int,lines[1].split())
	if len(set(l)) != len(l):
		print 'BAD'
		#print "\n".join(map(str,l))

def test_all():
	for i in range(10):
		test_single(1024<<i)
		test_single((1024<<i) * 5 / 4)
		test_single((1024<<i) * 7 / 4)

if __name__ == '__main__':
	if len(sys.argv) < 2:
		test_all()
	else:
		test_single(sys.argv[1])

