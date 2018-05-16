#!/bin/ksh
whoami
who am i
id
i=1
echo "ARG0: $0"
for a in "$@"
do
    echo "ARG$i: $a"
    let 'i=i+1'
done
