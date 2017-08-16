#!/bin/bash

#mycase="topo.txt"
#result="topo_result.txt"

mycase="case6.txt"
result="result6.txt"

rm cdn.tar.gz

./build.sh

cd ./bin


./cdn ../../case_example/3/$mycase ../../case_result/3/$result