
#!/bin/bash

case_path=/home/mio/Desktop/case_example/

rm cdn.tar.gz
rm record.txt
./build.sh
cd ./bin

for i in {4..4}
do
   for j in {0..0}
   do
       case_file=${case_path}${i}"/case"${j}".txt"
       echo $case_file
       ./cdn $case_file ${i}"--"${j}

   done
done

# rm cdn.tar.gz

# ./build.sh

# cd ./bin


# ./cdn ../../case_example/1/case0.txt 1-0