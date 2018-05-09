#!/bin/bash
itarations=100
rm -rf output.txt

for (( i=1; i<=$itarations; i++ ))
do
        command='./test_threads $1 | grep "matched" >>  output.txt'
        eval $command
done

echo 'Time'
#echo $abort $throuput
filename="output.txt"
while read -r line
do
        set -- $line

        if [[ $8 == 0 ]]; then
        continue
        fi

        echo $2
	time=$(($time + $2))
        no_of_lines=$(($no_of_lines + 1)) 
done < "$filename"

        time=$(($time / $no_of_lines))
echo 'Average Time = ' $time
