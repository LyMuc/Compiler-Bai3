#! /bin/bash

#! Test 5 la test tu them de test cac truong hop REPEAT UNTIL, cau lenh gan, kieu String, Bytes....

make

./parser ../test/example1.kpl | diff ../test/result1.txt -

./parser ../test/example2.kpl | diff ../test/result2.txt -

./parser ../test/example3.kpl | diff ../test/result3.txt -

./parser ../test/example4.kpl | diff ../test/result4.txt -

./parser ../test/example5.kpl 
