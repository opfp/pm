#!/bin/bash 

# base fail - no input 
# if [ -z $1 ]; then 
#     echo "Please pass an install location in your PATH. Exit." 
#     exit 1 
# fi 
# check for able to compile 
compilers=("gcc", "clang") 
compiler=""

for c in $compilers; do 
    if [ -n $(which $c) ]; then 
        compiler=$c 
        break 
    fi 
done

if [ -z $compiler ]; then 
    echo No C compiler found. Cannot compile from source. Exit. 
    exit 3 
fi 
# check for libsqlite3 
if [ -z "$(which sqlite3)" ]; then 
    echo Dependency sqlite3 not found. Exit. 
    exit 4 
fi 

# find directory in PATH to put symlink to bin 
path=$(echo $PATH)
pass="F"

exc_locs=("/usr/local/bin", "/usr/local", "/usr", $1) 
exc_loc=""

IFS=':'
for p in $path; do 
    # echo $i
    IFS=','
    for i in $exc_locs; do 
        if [ $i == $p ]; then 
            pass="T" 
            exc_loc=$p
            break 
        fi 
    done
    if [ $pass == "T" ]; then 
        break 
    fi 
    IFS=':'
done 

# echo $exc_loc
# exit 0 

if [ $pass != "T" ]; then 
    #echo $1 not in your PATH. Exit. 
    echo Could not find a suitable install locaiton. ./install.sh [LOCATION IN YOUR PATH]
    exit 2 
fi 

#binaries, config, db in 
#IFS=","


for d in $ins_locs; do 
    if [ -n $(ls $d) ]; then 
        ins_loc=$d 
        break 
    fi 
done

if [ -z $ins_loc ]; then 
    echo Error finding install location. Exit. 
    exit 5 
fi 
# check for already installed 
if [ -z $("ls "$ins_loc"/pm")]; then 
    ins_loc=$ins_loc"/pm"    
elif [ -z ] 



mk_ins="mkdir "$ins_loc"/pm" 
echo $mk_ins
eval $mk_ins
