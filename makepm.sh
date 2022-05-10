#!/bin/bash
# if [ -z $1 ]; then
#     out="pm_exec"
# else
#     out=$1
# fi
clang lib/pm.c lib/enc.c lib/cli.c lib/o_str.c libhydrogen/hydrogen.c -std=c11 -I lib -I libhydrogen -lsqlite3 -o bins/pm_exec
