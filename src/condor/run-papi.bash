#!/bin/bash -e

ls /usr/local/lib/

sudo sysctl -w kernel.perf_event_paranoid=1
/usr/local/bin/papi_avail -a
/usr/local/bin/papi_native_avail

grep NUMA=y /boot/config-`uname -r`
# numactl --hardware

./protocols.out
