#!/bin/bash

export SCRIPT_NAME=./data_filter
export DATA_SIZE=1M
export INPUT_DATA=/dev/urandom
export OUTPUT_DATA=/home/mike/OS/Hw1/Hw1/output.txt


./data_filter $DATA_SIZE $INPUT_DATA $OUTPUT_DATA

if [ $? -ne 0 ]
then 
  printf "\n----ERROR-----\n${SCRIPT_NAME}  failed.\n"
else
  printf "\n----SUCCESS---\n${SCRIPT_NAME} is done! \ncheck ${OUTPUT_DATA} for the output data.\n"
fi