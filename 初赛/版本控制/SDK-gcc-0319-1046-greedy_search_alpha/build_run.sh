#!/bin/bash

mycase="topo.txt"
result="topo_result.txt"

#mycase="case2.txt"
#result="result2.txt"

./build.sh

cd ./bin

./cdn ../../case_example/$mycase ../../case_result/$result