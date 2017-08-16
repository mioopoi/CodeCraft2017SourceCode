#!/bin/bash

#mycase="topo.txt"
#result="topo_result.txt"

mycase="case0.txt"
result="result0.txt"

./build.sh

cd ./bin

./cdn ../../case_example/$mycase ../../case_result/$result