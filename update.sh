#!/bin/bash 

update () { 
    echo "Updating "$oldcom" to "$ccom
    # echo "make.sh" 
    # eval "./make.sh" 
    # if (($? == 1)); then 
    #     exit 2 
    targ=$ccom 
    build
    cmd="echo "$ccom" > .pm_commit" 
    exit 0 
} 

build () { 
    # check for able to compile 
    #compilers=("gcc" "clang") 
    compilers=("clang" "gcc") 
    compiler=""

    for c in ${compilers[@]}; do 
        eval="type -p "$c 
        path=$($eval)
        #echo $eval 
        echo $path  

        if [ ! -z $path ]; then 
        #if [ ! -z $(type -p "$c") ]; then  
            compiler=$path 
            break 
        fi 
    done

    if [ -z $compiler ]; then 
        echo No C compiler found. Cannot compile from source. Exit. 
        exit 3 
    fi 

    # check for libsqlite3 
    # todo : check for library, not cli tool 
    deplibs=("sqlite3" "lcrypt") 
    for dep in $deplins; do
        if [ -z "$(ldconfig -p | grep "$dep" )" ]; then 
            echo "Dependency "$dep"not found. Exit." 
            exit 4 
        fi 
    done

    build_cmd=$compiler" lib/pm.c lib/enc.c lib/cli.c lib/o_str.c lib/pmsql.c lib/libhydrogen/hydrogen.c lib/sqlite3.c \
        #  -I lib -I lib/libhydrogen -lcrypt -o '"$targ"'  -std=c11 2> /dev/null "
        # -L/usr/lib/x86_64-linux-gnu 
    echo $build_cmd
    eval $build_cmd 
    if [ $? != 0 ] ; then 
        echo "Compile failed. Exit." 
        exit 5 
    fi 
}

oldcom="no commit found" 
ccom=$(git rev-parse --short HEAD) 

if [ ! -e .pm_commit ]; then 
    update
fi 

oldcom=$(cat .pm_commit) 

if [[ ! $ccom -eq $oldcom ]]; then 
    update
fi 

exit 1 