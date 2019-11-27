#!/bin/bash -e

#ls /usr/local/lib/

#sudo sysctl -w kernel.perf_event_paranoid=1
#/usr/local/bin/papi_avail -a
#/usr/local/bin/papi_native_avail

#grep NUMA=y /boot/config-`uname -r`
# numactl --hardware

sudo dmidecode -t cache

ulimit -s 16384

LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH ./bin/protocols.out
LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH ./bin/workingset.out
