#!/bin/bash

PATH=$PWD/../..:$PATH

VERBOSE=false
LOG=/tmp/ag-testlog.txt
TEST_OUTPUT=/tmp/ag-test-output.txt


status()
{
    STATUS=$1

    if $VERBOSE; then
        echo $STATUS
    fi

    echo $STATUS >> $LOG
}

fail()
{
    echo "== TEST $TEST FAILED ==" | tee -a $LOG
}


declare -i RETURN=0

rm $LOG

for TEST in $@; do
    status "== TEST $TEST START =="

    OPTS=""
    if [[ `basename $TEST` =~ ^deposition.* ]]; then
        OPTS="-d"
    fi

    austerus-verge $OPTS $TEST/gcode &> $TEST_OUTPUT
    RESULT=$?
    RETURN+=$RESULT

    if [ ! "$RESULT" -eq 0 ]; then
        echo "bad exit code $RESULT" &>> $LOG
        fail
        continue
    fi

    diff -u $TEST/output $TEST_OUTPUT &>> $LOG
    RESULT=$?
    RETURN+=$RESULT

    if [ ! "$RESULT" -eq 0 ]; then
        fail
        continue
    fi

    status "== TEST $TEST PASSED =="
done

if [ ! "$RETURN" -eq 0 ]; then
    echo "see log: $LOG"
fi

exit $RETURN

