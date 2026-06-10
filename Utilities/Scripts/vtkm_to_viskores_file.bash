#!/bin/bash

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

if [ $(echo in | sed -E 's/\b(in)\b/out/') = out ] ; then
  # GNU extensions
  wb='\b'
  we='\b'
elif [ $(echo in | sed -E 's/[[:<:]](in)[[:>:]]/out/') = out ] ; then
  # BSD extensions
  wb='[[:<:]]'
  we='[[:>:]]'
else
  echo "Cannot identify sed regex extensions for word boundaries."
  exit 1
fi

read -r -d '' SEDRULES << EOF
s/$wb(vtk-m|vtkm|vtk_m)$we/viskores/g
s/$wb(vtk-m|vtkm|vtk_m)_/viskores_/g
s/_(vtk-m|vtkm|vtk_m)$we/_viskores/g
s/_(vtk-m|vtkm|vtk_m)_/_viskores_/g
s/$wb(VTK-M|VTK-m|VTKm|Vtk-m|Vtkm)$we/Viskores/g
s/$wb(VTK-M|VTK-m|VTKm|Vtk-m|Vtkm)_/Viskores_/g
s/_(VTK-M|VTK-m|VTKm|Vtk-m|Vtkm)$we/_Viskores/g
s/_(VTK-M|VTK-m|VTKm|Vtk-m|Vtkm)_/_Viskores_/g
s/$wb(VTKM|VTK_M)$we/VISKORES/g
s/$wb(VTKM|VTK_M)_/VISKORES_/g
s/_(VTKM|VTK_M)$we/_VISKORES/g
s/_(VTKM|VTK_M)_/_VISKORES_/g
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
