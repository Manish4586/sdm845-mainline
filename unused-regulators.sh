#!/bin/bash

ALL_REGS="../regulators.txt"
TARGET="arch/arm64/boot/dts/qcom/sdm845-oneplus-common.dtsi"

echo "" > ../only-once.txt

while read reg; do
	#echo "checking $reg"
	#echo $reg: $(echo $instances | wc -l)
	if [ $(cat $TARGET | grep -e ".*$reg.*" | wc -l) -eq 1 ]; then
		echo "$reg occurs only once"
		echo $reg >> ../only-once.txt
	fi
done < $ALL_REGS
