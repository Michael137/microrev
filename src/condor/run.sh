#!/bin/bash -e

declare OUT_FILE=$1
declare CONFIG_NAME=$2
declare WITH_VICTIM=$3
declare EXE=$4

export PIN_ROOT=/opt/intel/pin
export PATH=$PIN_ROOT:$PATH

echo "Writing to ${OUT_FILE}..."
cmd="pin -t "hw3.so" -outfile ${OUT_FILE} -config ${CONFIG_NAME} -victim ${WITH_VICTIM} -- "$EXE" $5 $6"
echo "running ${cmd}"

eval "${cmd}"
