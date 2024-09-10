#!/bin/zsh

bdir="/Users/owen/cs/dev/pm/" 
tdir=$bdir"/tests/" 
tname="ukey"

if [ ! -e $tdir"/.targ_version" ]; then 
	echo "No test version (.targ_version) set" 
	exit 1
fi 

targ_version=pm$(cat $tdir"/.targ_version")

#command to invoke pm 
invoke_pm=$bdir"/bins/"$targ_version" "$tdir"/"$tname".conf"

# create empty db 
if [ -e $tdir"/"$tname".db" ]; then 
	cmd="rm "$tdir"/"$tname".db" 
	echo $cmd 
	eval $cmd  
fi 

cmd="sqlite3 "$tdir"/"$tname".db < "$bdir"/maketb.sql" 
echo $cmd 
eval $cmd  

if (( $? != 0 )); then 
	echo "Issue "$?" initializing database" 
	exit -1 
fi 

comp_ciphertxts=( "apples" "banannas" "citr0us" "da1ndel0on") 
upswds=("zenon" "yamahaa!68+1" "xylaph0ne" "walwrus" )

ctxt_len=${#comp_ciphertxts[*]}
ups_len=${#upswds[*]}

if (( ctxt_len != ups_len )); then 
	echo "Test issue: differing length of ciphertexts and passwords"
fi 

#ups_len=1

# set the ciphertexts 
for (( i=1; $i<=ups_len; i++ )); do 
	ctxt=${comp_ciphertxts[$i]}
	pswd=${upswds[$i]}
	cmd=$invoke_pm" set "$ctxt"_e -ctext '"$ctxt"' -pword '"$pswd"'"
	echo $cmd 
	eval $cmd 
	if (( $? != 0 )); then 
		echo "TEST FAILED: compliant cipher set "$c" pswd "$pswd" in unique key" 
		exit -1 
	fi 
done 

#exit
# get the entries and ensure they're correct 

for (( i=1; $i<=ups_len; i++ )); do 
	ctxt=${comp_ciphertxts[$i]}
	pswd=${upswds[$i]}
	cmd=$invoke_pm" get "$ctxt"_e  -pword '"$pswd"'"
	echo $cmd 
	retrieved=$(eval $cmd)
	#echo $retrieved
	if (( $? != 0 )); then 
		echo "TEST FAILED: with error "$?" compliant cipher get "$ctxt"_e pswd "$pswd" in unique key"
		exit -1 
	fi 

	if [[ $ctxt != $retrieved ]]; then 
		echo "TEST FAILED: compliant cipher get "$ctxt"_e returned "$retrieved" expected "$ctxt 
		exit -1 
	fi 
done 
