#!/bin/bash

for i in {1..8}
	do
	./bench.sh ${1}_${i} 128 $i 
	done | tee $file

