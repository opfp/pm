#!/bin/bash 

if [ "$1" == "-s" ]; then 
    if [ -z $2 ]; then 
        echo set: no version specified. 
        exit 1 
    fi 
    echo $2 > .pm_version
    if [ ! -e "bins/"$2 ]; then 
        echo New version detected. make.sh to build the binary before running pmd. 
    fi 
    exit 0 
fi 

if [ ! -e ".pm_version" ]; then 
    echo No PM version defined. Use -s to set 
    exit 2 
fi 

cat .pm_version