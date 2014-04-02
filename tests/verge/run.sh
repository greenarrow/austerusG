#!/bin/bash

PATH="${PWD}:${PWD}/../..:${PATH}"

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


usage()
{
    echo "Usage: $1 [OPTIONS] [TEST..]" >&2
    echo >&2
    echo "Options:" >&2
    echo "  -v  explain what is being done" >&2
    echo "  -h  display this help and exit" >&2
}

VERBOSE=false

while getopts 'hv' OPTION
do
    case $OPTION in
        h)
            usage "`basename $0`"
            exit 0
            ;;
        v)
            VERBOSE=true
            ;;
    esac
done

shift $(($OPTIND - 1))

declare -i RETURN=0

rm $LOG

for TEST in $@; do
    status "== TEST $TEST START =="

    OPTS=`cat "$TEST/flags"`

    if $VERBOSE; then
        echo austerus-verge $OPTS "$TEST/gcode"
    fi

    austerus-verge $OPTS "$TEST/gcode" &> $TEST_OUTPUT
    RESULT=$?
    RETURN+=$RESULT

    if [ ! "$RESULT" -eq 0 ]; then
        echo "bad exit code $RESULT" &>> $LOG
        fail
        continue
    fi

    if ($VERBOSE); then
        diff -u $TEST/output $TEST_OUTPUT
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

