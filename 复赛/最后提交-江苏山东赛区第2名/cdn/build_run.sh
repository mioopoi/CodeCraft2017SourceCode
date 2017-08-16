#!/bin/bash

#mycase="topo.txt"
#result="topo_result.txt"

mycase="case0.txt"
result="result0.txt"

rm cdn.tar.gz

./build.sh

cd ./bin


./cdn ../../case_example/4/$mycase ../../case_result/4/$result