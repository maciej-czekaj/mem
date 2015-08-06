#!/usr/bin/python
import struct
import sys

PG_SZ = 4096
PFN_MASK = (1 << 55) - 1

PAGEFLAGS  =  { 
	0 : 'LOCKED',
	1 : 'ERROR',
	2: 'REFERENCED',
	3: 'UPTODATE',
	4: 'DIRTY',
	5: 'LRU',
	6: 'ACTIVE',
	7: 'SLAB',
	8: 'WRITEBACK',
	9: 'RECLAIM',
	10: 'BUDDY',
	11: 'MMAP',
	12: 'ANON',
	13: 'SWAPCACHE',
	14: 'SWAPBACKED',
	15: 'COMPOUND_HEAD',
	16: 'COMPOUND_TAIL',
	16: 'HUGE',
	18: 'UNEVICTABLE',
	19: 'HWPOISON',
	20: 'NOPAGE',
	21: 'KSM',
	22: 'THP',
}

def readvm(va, size, pid):
	vpfn = va / PG_SZ
	va_off = vpfn * 8
	pg_num = size / PG_SZ + (1 if size % PG_SZ else 0)
	with open("/proc/%u/pagemap" % pid,"rb") as f:
		for pg in range(pg_num):
			off = va_off + pg * 8
			f.seek(off)
			pfn = struct.unpack("L", f.read(8))[0] & PFN_MASK
			flags, desc = readpf(pfn, 1)
			print "VA %x PA %x flags %x %s" % ((vpfn + pg) * PG_SZ, pfn * PG_SZ, flags, desc)

def readpf(pfn, pg_num):
	off = pfn*8
	with open("/proc/kpageflags","rb") as f:
		f.seek(off)
		for i in range(pg_num):
			flags = struct.unpack("L", f.read(8))[0]
			pf_desc = [PAGEFLAGS.get(x) for x in bits(flags, 64) if x in PAGEFLAGS]
			return flags, ' '.join(pf_desc)

def bits(val, n):
	a = []
	for i in range(n):
		if (1<<i) & val:
			a.append(i)
	return a

def main():
	if len(sys.argv) != 4:
		print "Usage:\n\treadvm.py <PID> <VIRT-ADDR> <SIZE>"
		print "example:\n\treadvm.py 1234 0x7f430000 0x1000"
		return
	pid = int(sys.argv[1])
	va = int(sys.argv[2],16)
	size = int(eval(sys.argv[3]))
	readvm(va, size, pid)

if __name__ == '__main__':
	main()
