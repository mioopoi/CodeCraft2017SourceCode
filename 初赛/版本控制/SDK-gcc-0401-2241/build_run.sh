#!/bin/bash

#mycase="topo.txt"
#result="topo_result.txt"

mycase="case3.txt"
result="result3.txt"

./build.sh

cd ./bin

./cdn ../../case_example/3/$mycase ../../case_result/3/$result