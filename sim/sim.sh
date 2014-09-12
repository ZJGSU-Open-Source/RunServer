#!/bin/bash
EXTENSION="$2"
FILE="$1/../Main.$EXTENSION"
for i in "$1"/*.$EXTENSION
do 
	sim=`../RunServer/sim/sim_$EXTENSION -p $FILE $i | grep -n ^"File $FILE" | awk '{print $3}'`
	
	if [ ! -z $sim ] && [ $sim -gt 50 ]
	then 
		sim_s_id=`basename $i`
		echo "$sim $sim_s_id" >sim
		exit $sim 
	fi
done
exit 0;
