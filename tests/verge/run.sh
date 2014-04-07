#!/bin/bash

PATH="`git rev-parse --show-toplevel`:${PATH}"

OUTPUT=`mktemp`
VERBOSE=false
FAIL=false

declare -i FAILURES=0


usage()
{
    echo "Usage: $1 [OPTIONS] [TEST..]" >&2
    echo >&2
    echo "Options:" >&2
    echo "  -v  explain what is being done" >&2
    echo "  -h  display this help and exit" >&2
}


while getopts 'hv' OPTION
do
    case "${OPTION}" in

        h)
            usage `basename "${0}"`
            exit 0
            ;;
        v)
            VERBOSE=true
            ;;
    esac
done

shift $((${OPTIND} - 1))

for TEST in $@; do
    FAIL=false
    OPTS=`cat "$TEST/flags"`

    if $VERBOSE
    then
        echo " START: ${TEST}" >&2
        echo "   RUN: austerus-verge ${OPTS} ${TEST}/gcode" >&2
    fi

    austerus-verge $OPTS "$TEST/gcode" > "${OUTPUT}"
    RC=$?

    if [ "${RC}" -ne 0 ]
    then
        if ${VERBOSE}
        then
            echo "bad exit code ${RC}" >&2
        fi

        FAIL=true
    fi

    if ${VERBOSE}
    then
        diff -u $TEST/output $OUTPUT >&2
    else
        diff -u $TEST/output $OUTPUT > /dev/null
    fi

    RC=$?

    if [ "${RC}" -ne 0 ]
    then
        FAIL=true
    fi

    if ${FAIL}
    then
        FAILURES+=1
        echo "FAILED: ${TEST}" >&2
    else
        if ${VERBOSE}
        then
            echo "PASSED: ${TEST}" >&2
        fi
    fi
done

if [ "${FAILURES}" -gt 0 ]
then
    if ${VERBOSE}
    then
        echo "${FAILURES} failures" >&2
    else
        echo "run in verbose mode for more details:" >&2
        echo "$0 -v $@" >&2
    fi
    exit 1
fi
