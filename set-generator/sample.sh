#!/usr/bin/env bash

SZ=1000000
SN=1000
ES=32
SEED=1574465740

chmod +x ./set-generator.py
./set-generator.py -s ${SZ} -n ${SN} -l ${ES} --seed ${SEED} -o ../reconciliation/test-sets/${SZ}-${SN}/sets_${SZ}_${SN}_${ES}_${SEED}.txt
