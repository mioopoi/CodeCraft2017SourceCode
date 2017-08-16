#!/bin/bash

mycase="topo.txt"
result="topo_result.txt"

#mycase="case4.txt"
#result="result4.txt"

./build.sh

cd ./bin

./cdn ../../case_example/$mycase ../../case_result/$result