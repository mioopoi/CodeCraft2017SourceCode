#!/bin/bash

#mycase="topo.txt"
#result="topo_result.txt"

mycase="case0.txt"
result="result0.txt"

./build.sh

cd ./bin

./cdn ../../case_example/2/$mycase ../../case_result/2/$result