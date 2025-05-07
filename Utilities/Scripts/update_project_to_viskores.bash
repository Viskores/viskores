#!/bin/bash

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

SCRIPT_DIRNAME=$(dirname "$0")

function usage()
{
  echo "Usage: $(basename "$0") [options] directory..."
  echo "Transition source files in a given directory from VTK-m to Viskores"
  echo "Options:"
  echo "    -j <JOBS> number of simultaneous processes to use."
  exit 0
}

function die()
{
  echo "ERROR: $*"
  usage
}

command -v nproc &> /dev/null || die "nproc needed"
command -v parallel &> /dev/null || die "GNU parallel needed"

NJOBS="$(nproc)"
while getopts "j:h" o
do
  case "${o}" in
    j)
      NJOBS="${OPTARG}"
      shift
      ;;
    h|*)
      usage
      ;;
  esac
done
shift $((OPTIND-1))
readonly PROJECT_PATH=("$@")

(( $# == 0 )) && die "input directory required"

# Get me all the non-binary files
sources=$(grep -rIl "." "${PROJECT_PATH[@]}")
parallel \
  --progress \
  -j"${NJOBS}" \
   "${SCRIPT_DIRNAME}/vtkm_to_viskores_file.bash -f {} > {}.tmp && mv {}.tmp {}" ::: "${sources[@]}"
