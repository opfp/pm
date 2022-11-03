#!/bin/bash 

# base fail - no input 
if [ -z $1 ]; then 
    echo "Please pass an install location in your PATH. Exit." 
    exit 1 
fi 
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

# check for target in path 
path=$(echo $PATH)
IFS=':'

pass="F"

for i in $path; do 
    # echo $i
    if [ $i == $1 ]; then 
        pass="T" 
    fi 
done 

if [ $pass != "T" ]; then 
    echo $1 not in your PATH. Exit. 
    exit 2 
fi 

# find directory to put binaries, config, db in 
IFS=","
ins_locs=("/usr/local/bin", "/usr/local", "/usr", $HOME)
ins_loc=""

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

if [ -z $("ls "$ins_loc"/pm")]; then 
    ins_loc=$ins_loc"/pm"    
elif [ -z ] 



mk_ins="mkdir "$ins_loc"/pm" 
echo $mk_ins
eval $mk_ins
