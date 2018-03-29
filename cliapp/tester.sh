#!/bin/bash


# Users to choose from
users=("Alice" "Bob" "Carol" "Dan" "Eve" "Fred" "Gwen" "Horace" "Iris")
to_comment=()

RANDOM=$$$(date +%s)

export VERSION=1

for X in {1..250}; do

   username=${users[$RANDOM % ${#users[@]}]}

   ./main-d.elf --fastrand --user=$username add --message="`fortune -l`" &> tmp

   guid="`grep \"Added new issue:\" tmp | cut -f 5- -d :`"

   if [ "$(($RANDOM % 5))" -eq 0 ]; then
      to_comment+=($guid)
   fi

   cat tmp

   cp issues.sitdb issues.sitdb.$VERSION
   VERSION=$(($VERSION + 1))

done

echo "--------------------------------------"

for X in ${to_comment[@]}; do

   echo Preparing to comment on $X
   username=${users[$RANDOM % ${#users[@]}]}

   ./main-d.elf --fastrand --user=$username comment $X --message="`fortune -l`"

   cp issues.sitdb issues.sitdb.$VERSION
   VERSION=$(($VERSION + 1))

done

