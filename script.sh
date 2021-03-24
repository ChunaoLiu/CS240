#!/bin/bash

read
NUM="$REPLY"
echo "$NUM"

for i in {1..100}
do
temp=$(make clean)
temp22=$(make)
grade=$(./hw"$NUM"_test 2>&1)
score=$(echo "$grade"|grep "Final points:")
if [ "$score" != "Final points: 100/100" ]
then echo "$grade" 
break 
else
echo -e "\n-------------------------------\nYou got 100 on this run. Noice!\n-------------------------------\n"
fi 
done
