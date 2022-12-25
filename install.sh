#!/bin/bash 

# BINLOC ,  INSLOC 

# check for able to compile 
compilers=("gcc" "clang") 
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
# todo : check for library, not cli tool 
deplibs=("sqlite3" "lcrypt") 
for dep in $deplins; do
    if [ -z "$(ldconfig -p | grep "$dep" )" ]; then 
        echo "Dependency "$dep"not found. Exit." 
        exit 4 
    fi 
done

# check for maketb 
if [ ! -e "maketb.sql" ]; then 
    echo "Dependency maketb.sql not found. Are you running from install stage dir?. Exit" 
fi 

# flags 
flags=("-b" "-l" "-n" )
fargs=("/usr/local/bin/" "/usr/local/etc/" "pm") 
fprint=("binary" "local files") 

argc=$# 

# process flags 
for ((f=0; f < 3; f++)); do 
    # echo "${flags[$f]}"
    for ((a=0; a < argc; a++)); do 
        if [[ "${!a}" == ${flags[$f]} && argc > a ]]; then 
            b=$((a+1)) 
            fargs[$f]="${!b}"
        fi  
    done 
    echo "${flags[f]}"" is ""${fargs[f]}" 
done 

# flags 0 and 1 must be valid filepaths 
for (( ck = 0; ck < 2; ck++ )); do 
    estr="Cannot place pm ""${fprint[ck]}"" in ""${fargs[ck]}"". %s. Specify alternate with ""${flags[ck]}"".\n" 
    #echo $estr 
    if [ ! -e "${fargs[ck]}" ]; then 
        printf "$estr" "Directory does not exist" 
        exit 1 
    fi
    fargs[$ck]=${fargs[$ck]}"/"${fargs[2]}
    # echo "${flags[ck]}"" is ""${fargs[ck]}" 
 
    if [ -e "$ins_name" ]; then 
        printf "$estr" "File already present"
        exit 1 
    fi 
done 

cmd="mkdir ""${fargs[1]}"
echo $cmd 
eval $cmd 

build_cmd=$compiler" lib/pm.c lib/enc.c lib/cli.c lib/o_str.c lib/pmsql.c lib/libhydrogen/hydrogen.c \
    -std=c11 -I lib -I lib/libhydrogen -lsqlite3 -lcrypt -o ""${fargs[1]}""/pm_bin"
echo $build_cmd
eval $build_cmd 
if [ $? != 0 ] ; then 
    echo "Compile failed. Exit." 
    exit 2 
fi 

cmd="touch ""${fargs[0]}" 
echo $cmd 
eval $cmd 

cmd="echo '#/bin/bash' > '""${fargs[0]}""'"
echo $cmd 
eval $cmd 

cmd="echo '""${fargs[1]}""/pm_bin ""${fargs[1]}""/pm.conf \$@' >> '""${fargs[0]}""'"
echo $cmd 
eval $cmd 

cmd="chmod +x ""${fargs[1]}""/pm_bin" 
echo $cmd 
eval $cmd 

cmd="chmod +x ""${fargs[0]}"
echo $cmd 
eval $cmd 


if [ ! -e "default.conf" ]; then 
    echo "pm.conf not created, as ./default.conf not found. you must add a pm.conf to be sourced manually. exit" 
else  
    cmd="cp ./default.conf ""${fargs[1]}""/pm.conf" 
    echo $cmd 
    eval $cmd 
fi 

cmd="echo 'db_path=""${fargs[1]}""/pswds.db'"" >> ""${fargs[1]}""/pm.conf" 
echo $cmd 
eval $cmd 

cmd="touch ""${fargs[1]}""/pswds.db"
echo $cmd 
eval $cmd 

cmd="sqlite3 ""${fargs[1]}""/pswds.db"" \"VACUUM;\"" 
echo $cmd 
eval $cmd 

cmd="sqlite3 ""${fargs[1]}""/pswds.db < maketb.sql" 
echo $cmd 
eval $cmd 
