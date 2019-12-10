#!/bin/bash -e

declare EXE=$1

# getconf -a
# lscpu

echo "1" | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH ./run.sh
echo "0" | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
