#!/bin/bash
EXTENSION="$2"
FILE="$1/../Main.$EXTENSION"

for i in "$1"/*.$EXTENSION
do 
	sim=`../RunServer/sim/sim_text -p -e -s $FILE $i | grep ^"$FILE" | awk '{print $4}'`
	if [ ! -z $sim ] && [ $sim -gt 50 ]
	then 
		sim_s_id=`basename $i | cut -d'.' -f1`
		echo "$sim $sim_s_id" > sim
		exit 0; 
	fi
done
exit 0;
