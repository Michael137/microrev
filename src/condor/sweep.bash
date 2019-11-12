#!/bin/bash -e

OUT_FILE=cacti-sweep.out

rm -f $OUT_FILE
touch $OUT_FILE

for i in 1 2 4 8 16
do
	TEMPLATE_FILE="cacti.sweep.${i}"
	sed "s|PLACEHOLDER|${i}|g" "cacti.config.template" > $TEMPLATE_FILE
	cacti -infile $TEMPLATE_FILE | grep -i "access time" | ASSOC=${i} awk '{ print ENVIRON["ASSOC"]","$4 }' >> "cacti-sweep.out"

	rm $TEMPLATE_FILE
done

cat $OUT_FILE
