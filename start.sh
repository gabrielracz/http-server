#!/bin/bash

PWD=/home/naoko/Projects/server/
COUNT=`expr $(cat $PWD/logs/.counter) + 1`
pkill -9 srv
echo -n $COUNT > $PWD/logs/.counter
$PWD/srv > logs/log$COUNT
