#!/usr/bin/env bash

FILE_PATH=../test/geometry-test-data/input/*

for f in $FILE_PATH
do
	echo "${f##*/}"
	for n in 10000 20000 30000 40000 50000 60000 70000 80000 90000 100000
	do
		../build/Release/vtile-encode $f 0 0 0 -i $n
	done
	echo " "
	echo " "
done