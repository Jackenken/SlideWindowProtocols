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


## Test Case 600
echo -n "Test case 600: Sending 600 packets and expecting receiver to print them out in order: "
(sleep 0.5; for i in `seq 1 600`; do echo "msg 0 0 Packet: $i"; sleep 0.1; done; sleep 5; echo "exit") | ./project3 -r 1 -s 1 > .output.600 2> .debug_output.600.log\

(for i in `seq 1 600`; do echo "<RECV0>:[Packet: $i]"; done) > .expected_output.600

check_output 600




echo "Completed test cases."

echo
