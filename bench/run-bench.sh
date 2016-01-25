#!/usr/bin/env bash

# encode geojson
if [ "$1" == "encode" ] ; then
	FILE_PATH=../test/geometry-test-data/input/*
	COMMAND=../build/Release/vtile-encode
#decode pbf
elif [ "$1" == "decode" ] ; then
	FILE_PATH=../test/data/**.pbf
	COMMAND=../build/Release/vtile-decode
else
	echo "Please specify 'encode' or 'decode'"
fi

# for each file in the directory, run the following command
for f in $FILE_PATH
do
	echo "${f##*/}"
	for n in 10000 20000 30000 40000 50000 60000 70000 80000 90000 100000
	do
		$COMMAND $f 0 0 0 -i $n
	done
	echo " "
	echo " "
done