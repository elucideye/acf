#!/bin/bash
#
# Suggested usage:
#
# cd acf
# ./bin/acfi-format.sh src



# Find all internal files, making sure to exlude 3rdparty subprojects
function find_acf_source()
{
    NAMES=(-name "*.h" -or -name "*.cpp" -or -name "*.hpp" -or -name "*.m" -or -name "*.mm")
    find $1 ${NAMES[@]}
}

input_dir=$1

echo ${input_dir}

find_acf_source ${input_dir} | grep -v "src/3rdparty" | while read name
do
    echo $name
    clang-format -i -style=file ${name}
done


