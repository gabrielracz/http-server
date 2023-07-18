#!/bin/bash

PWD=/home/naoko/Projects/server/
COUNT=`expr $(cat $PWD/logs/.counter) + 1`
pkill -9 http-server
echo -n $COUNT > $PWD/logs/.counter
$PWD/http-server &> logs/log$COUNT
