#!/bin/bash

if [ ! -e ".pm_version" ]; then 
    echo No pm version to build for. version.sh -s [VERSION]. 
fi 

sout="make.out"
targ=$(cat .pm_version)

if [ "$1" = "-v" ]; then
    sout="/dev/stdout"
fi

oldt=$(cat .pm_buildtime) 
#oldt=${oldt%' '*}

build_cmd="clang lib/pm.c lib/enc.c lib/cli.c lib/o_str.c lib/pmsql.c lib/libhydrogen/hydrogen.c \
    -std=c11 -I lib -I lib/libhydrogen -lsqlite3 -o bins/"$targ" 2> "$sout

echo $build_cmd
eval $build_cmd

newt=$(ls -i bins/$targ)
newt=${newt%' '*}

dif=$((newt-oldt))

if (( $dif > 0 ));
    then
    echo "BUILD SUCCESSFUl"
    echo $newt > .pm_buildtime  
    exit 0
else
    echo "BUILD FAILED"
    exit 1
fi

