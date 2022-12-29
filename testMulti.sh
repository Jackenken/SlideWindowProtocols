#!/bin/sh

check_output() {
    cmp .output.$1 .expected_output.$1 2>/dev/null >/dev/null
    if [ $? -eq 0 ]
    then
        echo "PASSED"
    else
        echo "FAILED"
        echo "Output from project3 binary is in file: .output.$1"
        echo "Expected output is in file: .expected_output.$1"
        echo "Debug output from project3 binary is in file: .debug_output.$1"
    fi
    echo
}


echo
echo
echo "Rebuilding project3..."
make clean; make -j4 project3
echo "Done rebuilding project3"
echo

## Test Case multi
s=2;
r=2;
echo -n "Test case multi: Sending 10 packets (with corrupt probability of 20% ,sender number of 3,receiver number of 3 and drop probability of 20%) and expecting receiver to print them out in order: "
(for m in `seq 0 $s`;
do
    for n in `seq 0 $r`;
    do
        sleep 0.5;
        for i in `seq 1 10`;
            do echo "msg $m $n Packet:$i";
        sleep 0.1;
        done;
        
    done;
done;
sleep 5;
echo "exit";) | ./project3 -d 0.2 -c 0.2 -r 3 -s 3 > .output.multi 2> .debug_output.multi.log

(   for m in `seq 0 $s`;
    do
        for n in `seq 0 $r`;
        do
            for i in `seq 1 10`; 
            do echo "<RECV$n>:[Packet:$i]"; 
            done;
        done
    done
) > .expected_output.multi

check_output multi





echo
echo "Completed test cases"

