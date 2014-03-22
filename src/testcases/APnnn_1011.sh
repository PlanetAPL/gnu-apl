#!/bin/bash

cd /home/eedjsa/projects/juergen/apl-1.1/src/

LOG1=testcases/APnnn_1011.tc2.log1
LOG2=testcases/APnnn_1011.tc2.log2
RUN=testcases/APnnn_1011-running

if [ "x$1" = "xrun" ]; then
   date > $LOG2
   rm $RUN
   ./apl --id 1011 --noCONT -f testcases/APnnn_1011.tc2
   echo "./apl done at `date`"
   date > $RUN
   exit 0
fi

date > $LOG1

while true; do

   $0 run >> $LOG2  2>&1 &
   sleep 1
   if [ ! -f $RUN ]; then
      echo "./apl --id 1011 succeeded at `date`" >> $LOG1
      exit 0
   fi

   echo "./apl --id 1011 failed    at `date`" >> $LOG1
done

date >> $LOG1

