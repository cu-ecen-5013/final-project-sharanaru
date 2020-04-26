#!/bin/bash
ip_subnet=10.0.0
for ((hostno=0; hostno<255; hostno++))
do
	if [ "$hostno" == 20 ]
	then
		server_address="$ip_subnet.$hostno"
		echo $server_address			
		break
	fi
echo "wait"
done

