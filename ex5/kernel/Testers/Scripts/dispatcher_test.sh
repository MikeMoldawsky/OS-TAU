#!/bin/bash
printf "\nInput is in the format:\nargv[1]={r | {w | s} }\n"
printf "if read: tuples of\t<channel, filename> ......<channel, filename>\n"
printf "if write: triples of\t<channel, msg, filename> ...... <channel, msg, filename>\n"

p="/dev/msd" # message slot device path

c=0
i=1
channels=4
while [  $c -lt $channels ]; do
		printf "\n\n######### round number ${i}: READ B4 Write (crap in buffer) CHANNEL ${c} ##############\n\n"
		./dispatcher r "${c}"  "${p}1" "${c}"  "${p}2" "${c}"  "${p}3" "${c}" "${p}4" "${c}"  "${p}5" "${c}" "${p}6"
		let c=c+1 
done

c=0 #channel
while [  $c -lt $channels ]; do
	printf "\n\n############## WRITE TO CHANNEL ${c} ##############\n\n"
	
	m="MSG passed in channel ${c} to device /dev/msd"
	./dispatcher w "${c}" "${m}1" "${p}1" "${c}" "${m}2" "${p}2" "${c}" "${m}3" "${p}3" "${c}" "${m}4" "${p}4" "${c}" "${m}5" "${p}5" "${c}" "${m}6" "${p}6"

	let c=c+1 
 done

c=0
i=1
rounds=3
while [  $i -lt $rounds ]; do
	while [  $c -lt $channels ]; do
		printf "\n\n############## round number ${i}: READ FROM CHANNEL ${c} ##############\n\n"
		./dispatcher r "${c}"  "${p}1" "${c}"  "${p}2" "${c}"  "${p}3" "${c}" "${p}4" "${c}"  "${p}5" "${c}" "${p}6"
		let c=c+1 
 	done
	let i=i+1
	c=0
done


printf "\n\n############## DONE!!!  SHOULD HAVE WROTE TO 4 CHANNELS and READ 2 TIMES ALL OF THEM##############\n\n"