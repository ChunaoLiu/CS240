#!/bin/bash

read
NUM="$REPLY"
echo "$NUM"

for i in {1..100}
do
temp=$(make clean)
temp22=$(make test_p"$NUM")
grade=$(./test_p"$NUM" 2>&1)
score=$(echo "$grade"|grep "Final score:")
if [ "$score" != "Final score: 25/25" ]
then echo "$grade"
break
else
echo -e "\n-------------------------------\nYou got 100 on this run. Noice!\n-------------------------------\n"
fi
done
