#!/bin/bash

# More info here: https://easyperf.net/blog/2018/06/01/PMU-counters-and-profiling-basics

CPUID=$(command -v cpuid)
if [ ! -z ${CPUID} ]; then
	cpuid | grep -m 1 -A20 "Performance Monitoring Features"
else
	echo "Command \"cpuid\" not found. Usign \"dmesg\" instead"
	dmesg | grep "Intel PMU driver" -A20
fi
