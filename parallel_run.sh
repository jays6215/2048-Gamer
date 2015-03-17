#!/bin/bash
# 7 parameter
# $ --------------- TrainTimes Method Chooser Updater Cumulative alpah beta setPrint setTest
# $ parallel_run.sh        300      1       4       1          0 0.0100 1.0        0       0
echo parallel_run.sh TrainTimes Method Chooser Updater Cumulative alpah beta setPrint setTest
make
program='a.out'
train_ts=$1
method=$2
chooser_m=$3
update_m=$4
cumulative=$5
alpha=$6
beta=$7
set_print=$8
set_test=$9
id_end=30
# Checking Parameters..
echo Exe:               $program
echo Training Times:    $train_ts
echo Method:            $method
echo Choose Method:     $choose_m
echo Update Method:     $update_m
echo Cumulative Mode:   $cumulative
echo alpha:             $alpha
echo beta:              $beta
echo setPrint           $set_print
echo setTest            $set_test
for (( i=1 ; i<=id_end ; i=i+1 ))
do
    #echo Exp ID is $i.
    sleep 0.123
    ./$program $i $train_ts $method $chooser_m $update_m $cumulative $alpha $beta $set_print $set_test &>/dev/null 2>&1 &
done
#./$program 1 $train_ts $method $chooser_m $update_m $cumulative $alpha $beta $set_print $set_test &
#./$program 2 $train_ts $method $chooser_m $update_m $cumulative $alpha $beta $set_print $set_test &
#/$program $1 $2 $3 $4 $5 $6 $7
