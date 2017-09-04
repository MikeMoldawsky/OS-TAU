#!/bin/bash

i=1
nodes=7
last=6
while [  $i -lt $nodes ]; do
	mknod /dev/msd${i} c 245 0
     	chmod 777 /dev/msd${i}
	let i=i+1 
 done

printf "######\n MKNOD CREATED FILES from /dev/msd1 .... /dev/msd${last} ################\n"

