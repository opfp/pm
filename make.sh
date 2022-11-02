#!/bin/bash

sout="pmerr.txt"
targ="latest"

if [ "$1" = "-v" ]; then
    sout="/dev/stdout"
fi

oldt=$(ls -i bins/$targ)
oldt=${oldt%' '*}

clang lib/pm.c lib/enc.c lib/cli.c lib/o_str.c lib/pmsql.c libhydrogen/hydrogen.c \
    -std=c11 -I lib -I libhydrogen -lsqlite3 -o bins/$targ 2> $sout

newt=$(ls -i bins/$targ)
newt=${newt%' '*}

dif=$((newt-oldt))

if (( $dif > 0 ));
    then
    echo "BUILD SUCCESSFUl"
    exit 0
else
    echo "BUILD FAILED"
    exit 1
fi
