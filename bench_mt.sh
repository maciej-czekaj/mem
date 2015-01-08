#!/bin/bash

i=1
while [ $i -le $2 ]
	do
	./bench.sh ${1}_${i} 128 $i -w
	i=$((i+1))
	done

