#!/bin/bash

#mycase="topo.txt"
#result="topo_result.txt"

mycase="case1.txt"
result="result1.txt"

./build.sh

cd ./bin

./cdn ../../case_example/$mycase ../../case_result/$result