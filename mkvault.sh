#echo "$1 $2"

tests=("mkvault hello;ls")
exout=(";hello")
IFS=";"

for i in ${tests[@]}; do
    echo "${1} ${2} "pm" "${tests[$i]}" :" >> pmerr.txt
    IFS=" "
    ret=$($1 $2 ${tests[$i]})
    if [ "$?" != "0" ]; then
        exit 1
    elif [ "$ret" != "${exout[$i]}" ]; then
        exit 2
    fi
    echo $ret >> pmerr.txt
    IFS=";"
done
