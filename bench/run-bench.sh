#!/usr/bin/env bash

# encode geojson
if [ "$1" == "encode" ] ; then
	FILE_PATH=./test/geometry-test-data/input/*
	COMMAND=./build/Release/vtile-encode
	for f in $FILE_PATH
	do
		echo "${f##*/}"
		for n in 10000 20000 30000 40000 50000 60000 70000 80000 90000 100000
		do
			DYLD_LIBRARY_PATH=$MVT_LIBRARY_PATH $COMMAND $f 0 0 0 -i $n
		done
		echo " "
		echo " "
	done
#decode pbf
elif [ "$1" == "decode" ] ; then
	FILE_PATH=./test/data/**.pbf
	COMMAND=./build/Release/vtile-decode
	for f in $FILE_PATH
	do
		echo "${f##*/}"
		for n in 1000 2000 3000 4000 5000 6000 7000 8000 9000 10000
		do
			DYLD_LIBRARY_PATH=$MVT_LIBRARY_PATH $COMMAND $f 0 0 0 $n
		done
		echo " "
		echo " "
	done
else
	echo "Please specify 'encode' or 'decode'"
fi
