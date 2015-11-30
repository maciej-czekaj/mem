#!/usr/bin/env python
import sys
import math
from collections import defaultdict

class Stats(object):

	def __init__(self):
		self.samples =  []
	
	def compute(self, precision=None):
		self.samples.sort()
		self._avg()
		self._var()
		self._stdev()
		if not precision:
			precision = self.stdev / 50
		self._histogram(precision)

	def _avg(self):
		sum_samples = sum(self.samples, 0)
		self.avg = sum_samples/len(self.samples)
	
	def _var(self):
		avg = self.avg
		sum_dev = sum(((avg - x)**2 for x in self.samples),0)
		self.var = sum_dev/(len(self.samples) - 1)

	def _stdev(self):
		self.stdev = math.sqrt(self.var)

	def _histogram_1(self, precision):
		hist = [0]
		bucket = self.samples[0] + precision
		for x in self.samples:
			if x >= bucket:
				bucket += precision
				hist.append(1)
			else:
				hist[-1] += 1

	def _histogram(self, precision):
		hist = defaultdict(lambda: 0)
		for x in self.samples:
			x_rounded = int(math.floor(x/precision))
			hist[x_rounded] = hist[x_rounded] + 1
		self.histogram = [(x*precision,y) for x,y in hist.iteritems()]
		self.histogram.sort()

def stats(file_in, file_out, sumfile=None):
	stats = Stats()
	while True:
		line = file_in.readline()
		if line == '':
			break;
		if line.strip() == '':
			for i in range(5):
				file_in.readline()
			continue
		try:
			x = float(line)
		except ValueError,e:
			print >> sys.stderr, e
			exit(1)
		stats.samples.append(x)
	stats.compute()
	if sumfile:
		print >> sumfile, '%f %d\n%f %d\n %f %d\n' % (
			stats.avg - stats.stdev, 1,
			stats.avg, 1,
			stats.avg + stats.stdev, 1)
		print >> sys.stderr, 'Avg = %f\nStdDev = %f\n' % (stats.avg, stats.stdev)
	else:
		print 'Avg = %f\nStdDev = %f\n' % (stats.avg, stats.stdev)
	print >> file_out, '\n'.join(('%f %d' % (x,y) for x,y in stats.histogram))

def main():
	out_fname = sys.argv[1]
	sum_file = None
	if (len(sys.argv) > 2):
		sum_fname = sys.argv[2]
		sum_file = open(sum_fname,'wb')
	out_file = open(out_fname,'wb')
	try:
		stats(sys.stdin, out_file, sum_file)
	finally:
		out_file.close()
		sum_file.close() if sum_file else None

if __name__ == '__main__':
	main()

