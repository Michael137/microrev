#!/bin/bash -e

rm -f config-base-*
rm -f measure-log-*
rm -f config.submit.*
rm -f out/*

## Without victim cache
for i in 1 2 4 8
do
	TEMPLATE="config.submit-template"
	SUBMIT_NAME="config.submit.${i}"
	CONFIG_NAME="config-base-${i}"
	LOG_NAME="measure-log-${i}"
	OUT_FILE="out/cache_assoc_${i}.out"
	sed "s|PLACEHOLDER|${i}|g" config-base.template > $CONFIG_NAME
	sed -i "s|VICTIM|1|g" $CONFIG_NAME
	sed "s|CONFIG|${CONFIG_NAME}|g" $TEMPLATE > $SUBMIT_NAME
	sed -i "s|LOG|${LOG_NAME}|g" $SUBMIT_NAME
	sed -i "s|ARGUMENTS|${OUT_FILE} ${CONFIG_NAME} 0 /usr/local/benchmarks/libquantum_O3 400 25|g" ${SUBMIT_NAME}

	condor_submit $SUBMIT_NAME
done

# With victim cache
for i in 1 2 3 4 5 6 7 8
do
	TEMPLATE="config.submit-template"
	SUBMIT_NAME="config.submit.${i}-victim"
	CONFIG_NAME="config-base-${i}-victim"
	LOG_NAME="measure-log-${i}-victim"
	OUT_FILE="out/victim_${i}_l1_direct.out"
	sed "s|PLACEHOLDER|1|g" config-base.template > $CONFIG_NAME
	sed -i "s|VICTIM|${i}|g" $CONFIG_NAME
	sed "s|CONFIG|${CONFIG_NAME}|g" $TEMPLATE > $SUBMIT_NAME
	sed -i "s|LOG|${LOG_NAME}|g" $SUBMIT_NAME
	sed -i "s|ARGUMENTS|${OUT_FILE} ${CONFIG_NAME} 1 /usr/local/benchmarks/libquantum_O3 400 25|g" ${SUBMIT_NAME}

	condor_submit $SUBMIT_NAME
done

# With victim cache
for i in 1 2 3 4 5 6 7 8
do
	TEMPLATE="config.submit-template"
	SUBMIT_NAME="config.submit.${i}_l8_direct-victim"
	CONFIG_NAME="config-base-${i}_l8_direct-victim"
	LOG_NAME="measure-log-${i}_l8_direct-victim"
	OUT_FILE="out/victim_${i}_l8_direct.out"
	sed "s|PLACEHOLDER|8|g" config-base.template > $CONFIG_NAME
	sed -i "s|VICTIM|${i}|g" $CONFIG_NAME
	sed "s|CONFIG|${CONFIG_NAME}|g" $TEMPLATE > $SUBMIT_NAME
	sed -i "s|LOG|${LOG_NAME}|g" $SUBMIT_NAME
	sed -i "s|ARGUMENTS|${OUT_FILE} ${CONFIG_NAME} 1 /usr/local/benchmarks/libquantum_O3 400 25|g" ${SUBMIT_NAME}

	condor_submit $SUBMIT_NAME
done
