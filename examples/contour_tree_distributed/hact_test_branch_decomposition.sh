#!/bin/sh

##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

GTCT_DIR=${GTCT_DIR:-${HOME}/devel/parallel-peak-pruning/ContourTree/SweepAndMergeSerial/out}
RED=""
GREEN=""
NC=""
if [ -t 1 ]; then
# If stdout is a terminal, color Pass and FAIL green and red, respectively
    RED=$(tput setaf 1)
    GREEN=$(tput setaf 2)
    NC=$(tput sgr0)
fi

echo "Removing previously generated files"
rm *.log *.dat

echo "Copying target file "$1 "into current directory"
filename=${1##*/}
fileroot=${filename%.txt}

cp $1 ${filename}

echo "Splitting data into "$2" x "$2" parts"
./split_data_2d.py ${filename} $2
rm ${filename}

echo "Running HACT"
n_parts=$(($2*$2))
# mpirun -np 4 --oversubscribe ./ContourTree_Distributed --vtkm-device Any --preSplitFiles --saveOutputData --augmentHierarchicalTree --computeVolumeBranchDecomposition --numBlocks=${n_parts} ${fileroot}_part_%d_of_${n_parts}.txt
mpirun -np 2 --oversubscribe ./ContourTree_Distributed --vtkm-device Any --preSplitFiles --saveOutputData --augmentHierarchicalTree --computeVolumeBranchDecomposition --numBlocks=${n_parts} ${fileroot}_part_%d_of_${n_parts}.txt
rm ${fileroot}_part_*_of_${n_parts}.txt

echo "Compiling Outputs"
sort -u BranchDecomposition_Rank_*.txt > outsort${fileroot}_$2x$2.txt
cat outsort${fileroot}_$2x$2.txt | ./BranchCompiler | sort > bcompile${fileroot}_$2x$2.txt
rm BranchDecomposition_Rank_*.txt outsort${fileroot}_$2x$2.txt

echo "Diffing"
echo diff bcompile${fileroot}_$2x$2.txt ${GTCT_DIR}/branch_decomposition_volume_hybrid_${fileroot}.txt
diff bcompile${fileroot}_$2x$2.txt ${GTCT_DIR}/branch_decomposition_volume_hybrid_${fileroot}.txt

if test $? -eq 0; then echo "${GREEN}Pass${NC}"; rm bcompile${fileroot}_$2x$2.txt; else echo "${RED}FAIL${NC}"; fi;

# echo "Generating Dot files"
# ./makedot.sh
