#!/bin/zsh
#test_dir="$(dirname "$0")"
bdir="/Users/owen/cs/dev/pm/" 
tdir=$bdir"/tests/" 

if [ ! -e $tdir"/.targ_version" ]; then 
	echo "No test version (.targ_version) set" 
	exit 1
fi 

targ_version=pm$(cat $tdir"/.targ_version")

#command to invoke pm 
invoke_pm=$bdir"/bins/"$targ_version" "$tdir"/test.conf"

# create empty db 
if [ -e $tdir/pswds.db ]; then 
	cmd="rm "$tdir"/pswds.db" 
	echo $cmd 
	eval $cmd  
fi 

cmd="sqlite3 "$tdir"/pswds.db < "$bdir"/maketb.sql" 
echo $cmd 
eval $cmd  

if (( $? != 0 )); then 
	echo "Issue "$?" initializing database" 
	exit -1 
fi 

comp_ciphertxts=( "apples" "banannas" "citr0us" "da1ndel0on") 
comp_pswd="very_secure_pswd" 
upswds=("zenon" "yamahaa!68+1" "xylaph0ne" "walwrus" )


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

exit 

ctxt_len=${#comp_ciphertxts[*]}
ups_len=${#upswds[*]}
if (( ctxt_len != ups_len )); then 
	echo "Test issue: differing length of ciphertexts and passwords"
fi 

for (( i=1; $i<=ups_len; i++ )); do 
	ctxt=${comp_ciphertxts[$i]}
	pswd=${upswds[$i]}
	cmd=$invoke_pm" set "$ctxt"_e -ctext '"$ctxt"' -pword '"$pswd"' --u"
	echo $cmd 
	eval $cmd 
	if (( $? != 0 )); then 
		echo "TEST FAILED: compliant cipher set "$c" pswd "$pswd" in unique key" 
		exit -1 
	fi 
done 

#function set { 
#	cmd=$bdir"/pmd set "$1" -password '"$2"'"
#	echo $cmd 
#	#eval cmd
#} 
