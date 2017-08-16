#!/bin/bash

mycase="topo.txt"
result="topo_result.txt"

#mycase="case50.txt"
#result="result50.txt"

./build.sh

cd ./bin

./cdn ../../case_example/$mycase ../../case_result/$result