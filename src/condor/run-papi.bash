#!/bin/bash -e

declare EXE=$1

# getconf -a
# lscpu

LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH ./run.sh
