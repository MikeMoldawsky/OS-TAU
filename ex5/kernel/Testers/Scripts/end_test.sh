#!/bin/bash

rm /dev/msd*
rmmod message_slot.ko

make clean

printf "\n\n###############MODULE And Device files Deleted############\n\n"

