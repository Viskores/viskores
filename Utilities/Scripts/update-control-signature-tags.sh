#!/bin/sh

extentions_to_change="cxx cpp cu h hxx hpp"

tags_to_change=" \
    FieldIn
    FieldOut
    FieldInOut
    FieldInTo
    FieldInFrom
    FieldInCell
    FieldOutCell
    FieldInOutCell
    FieldInPoint
    FieldInOutPoint
    FieldOutPoint
    FieldInNeighborhood
    ValuesIn
    ValuesInOut
    ValuesOut
    ReducedValuesIn
    ReducedValuesInOut
    ReducedValuesOut
    WholeArrayIn
    WholeArrayOut
    WholeArrayInOut
    AtomicArrayInOut
"

if [ $# -ne 1 ]
then
    echo "USAGE: $0 <directory>"
    echo
    echo "This script seaches for C++ source files and removes the template"
    echo "arguments of select ControlSignature tags that have changed since"
    echo "VTK-m 1.3. This script searches through all subdirectories of the"
    echo "given directory. Files with the following extensions are processed:"
    echo
    echo "   $extentions_to_change"

    exit 1
fi

find_command="find '$1' -name 'not-a-name'"

for extention in $extentions_to_change
do
    find_command="$find_command -o -name '*.$extention'"
done

sed_command="sed"

for tag in $tags_to_change
do
    sed_command="$sed_command -e 's/\\([^a-zA-Z]\\)$tag<[^<>,]*>/\\1$tag/g'"
    sed_command="$sed_command -e 's/\\([^a-zA-Z]\\)$tag<[^<>,]*<[^<>]*>[^<>,]*>/\\1$tag/g'"
    sed_command="$sed_command -e 's/\\([^a-zA-Z]\\)$tag<[^<>,]*<[^<>]*<[^<>]*>[^<>]>[^<>,]*>/\\1$tag/g'"
done

echo -n "Converting files in `realpath $1`"
for file in `eval $find_command`
do
    eval $sed_command $file > $file._do_update_sig
    if diff $file $file._do_update_sig > /dev/null
    then
        rm $file._do_update_sig
    else
        rm $file
        mv $file._do_update_sig $file
    fi
    echo -n "."
done

echo done
