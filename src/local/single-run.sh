#!/bin/bash

EXE=$1

if [ -z "${EXE}" ]; then
	echo "Please provide executable name to run. E.g., \"benchmark\""
	exit 1
fi

if [[ "$OSTYPE" == "freebsd"* ]]; then
	gmake run EXE=${EXE}
elif [[ "$OSTYPE" == "linux-gnu" ]]; then
	make run EXE=${EXE}
fi
