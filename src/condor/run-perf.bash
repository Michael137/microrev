#!/bin/bash -e

declare events=$1
declare exe=$2

rm -rf out
mkdir out

# sudo perf list
# cat /proc/cpuinfo > out/procinfo.out
# sudo lshw -C memory > out/lshw.out
# lscpu > out/lscpu.out
# x86info -c > out/x86info.out

# sudo dmidecode -t cache > out/dmidecode.out

sudo perf stat -e "$events" /usr/local/benchmarks/libquantum_O3 400 25 3>&1 >/dev/null 2>out/quantum.out
# sudo perf stat -e "$events" /usr/local/benchmarks/hmmer_O3 /usr/local/benchmarks/inputs/nph3.hmm /usr/local/benchmarks/inputs/swiss41 3>&1 >/dev/null 2>out/hmmer.out
