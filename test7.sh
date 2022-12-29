#!/bin/sh

check_output() {
    cmp .output.$1 .expected_output.$1 2>/dev/null >/dev/null
    if [ $? -eq 0 ]
    then
        echo "PASSED"
        #rm $1 $2
    else
        echo "FAILED"
        echo "Output from project3 binary is in file: .output.$1"
        echo "Expected output is in file: .expected_output.$1"
        echo "Debug output from project3 binary is in file: .debug_output.$1.log"
    fi
    echo
}

echo
echo
echo "Rebuilding project3..."
make clean; make -j4 project3
echo "Done rebuilding project3"
echo


## Test Case 7
echo -n "Test case 7: Sending 10 packets (with corrupt probability of 20% ,drop probability of 20% and the message length more than FRAME_PAYLOAD_SIZE) and expecting receiver to print them out in order:"
(sleep 0.5;
for i in `seq 1 10`;
do echo "msg 0 0 Packet: $i Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World";
sleep 0.1; done; sleep 5; echo "exit") | ./project3 -d 0.2 -c 0.2 -r 1 -s 1 > .output.7 2> .debug_output.7.log\

(for i in `seq 1 10`; do echo "<RECV0>:[Packet: $i Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World Hello World]"; done) > .expected_output.7

check_output 7


echo "Completed test cases."

echo
