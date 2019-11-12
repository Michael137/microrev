#!/usr/local/bin/bash

EXE=$1

if [ -z "${EXE}" ]; then
	echo "Please provide executable name to run. E.g., \"benchmark\""
	exit 1
fi

gmake run EXE=${EXE}
