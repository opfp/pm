#!/bin/zsh
#test_dir="$(dirname "$0")"
bdir="/Users/owen/cs/dev/pm/" 
tdir=$bdir"/tests/" 
tname="basic"

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
#comp_ciphertxts=( "apples" )
comp_pswd="very_secure_pswd" 
#upswds=("zenon" "yamahaa!68+1" "xylaph0ne" "walwrus" )

#set some compliant ciphertexts in common key (note that name ctext+_e here)  
for c in $comp_ciphertxts; do 
	cmd=$invoke_pm" set "$c"_e -ctext '"$c"' -pword '"$comp_pswd"'"
	echo $cmd 
	eval $cmd 
	if (( $? != 0 )); then 
		echo "TEST FAILED: compliant cipher set "$c" in common key" 
		exit -1 
	fi 
done 

# get the entries and ensure they're correct 

for c in $comp_ciphertxts; do 
	cmd=$invoke_pm" get "$c"_e  -pword '"$comp_pswd"'"
	echo $cmd 
	retrieved=$(eval $cmd | tr -d '\n')

	if (( $? != 0 )); then 
		echo "TEST FAILED: with error "$?" compliant cipher get "$c"_e pswd '"$comp_pswd"'"
		exit -1 
	fi 

	if [[ $c != $retrieved ]]; then 
		echo "TEST FAILED: compliant cipher get "$c"_e returned '"$retrieved"' expected '"$c"'"
		exit -1 
	fi 
done 
