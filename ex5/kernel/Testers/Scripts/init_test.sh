#!/bin/bash
gcc dispatcher.c -o dispatcher
gcc message_reader.c -o reader
gcc message_sender.c -o sender
make clean
make
insmod message_slot.ko
printf "\n############### MODULE INSERTED ############\n"


