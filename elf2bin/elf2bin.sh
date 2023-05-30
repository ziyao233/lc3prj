#!/bin/sh

if [ $1x = x ] || [ $2x = x ]
then
	echo "Usage: $0 ELF_File BinaryFile"
	exit 1
fi

tail -c +181 $1 > $2
