#!/bin/bash

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


read -r -d '' SEDRULES << EOF
s/vtk-m|vtkm|vtk_m/viskores/g
s/VTK-M|VTK-m|VTKm|Vtk-m|Vtkm/Viskores/g
s/VTKM|VTK_M/VISKORES/g
s/vtkmdiy/viskoresdiy/g
EOF

function usage()
{
  echo "Usage: $0 [-f <TARGET_FILE>]"
  echo "    -f <TARGET_FILE> Translation unit where to perform the translation"
  exit 0
}

function die()
{
  echo "ERROR: $*"
  usage
}


while getopts "f:" o
do
  case "${o}" in
    f)
      TARGET_FILE="${OPTARG}"
      ;;
    *)
      usage
      ;;
  esac
done

test -z "$TARGET_FILE" && die "TARGET_FILE parameter required"

sed -E -f <(echo "${SEDRULES}") "${TARGET_FILE}"
