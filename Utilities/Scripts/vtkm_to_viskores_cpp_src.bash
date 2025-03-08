#!/bin/bash

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

SCRIPT_DIRNAME=$(dirname "$0")

read -r -d '' CPPRULES << EOF
---
- QualifiedName: vtk-m
  NewName: viskores
- QualifiedName: vtkm
  NewName: viskores
- QualifiedName: Vtk-m
  NewName: Viskores
- QualifiedName: Vtkm
  NewName: Viskores
- QualifiedName: VTK-m
  NewName: Viskores
- QualifiedName: VTKm
  NewName: Viskores
- QualifiedName: VTK-M
  NewName: Viskores
- QualifiedName: VTKM
  NewName: VISKORES
- QualifiedName: VTK_M
  NewName: VISKORES
- QualifiedName: vtkmdiy
  NewName: viskoresdiy
...
EOF

read -r -d '' SEDRULES << EOF
# PREPROCESSOR
/^#/s/vtk-m|vtkm|vtk_m/viskores/g
/^#/s/VTK-M|VTK-m|VTKm|Vtk-m|Vtkm/Viskores/g
/^#/s/VTK_M|VTKM/VISKORES/g
s/VTKm/Viskores/g
s/VTKM|VTK_M/VISKORES/g

# C STYLE COMMENTS
/\/\*.*\*\//s/vtk-m|vtkm/viskores/g
/\/\*.*\*\//s/VTK-M|VTK-m|VTKm|Vtk-m|Vtkm/Viskores/g
/\/\*.*\*\//s/VTKM|VTK_M/VISKORES/g

# CPP STYLE COMMENTS
s/(\/\/\/?.*)(vtk-m|vtkm)/\1viskores/g
s/(\/\/\/?.*)(VTK-M|VTK-m|VTKm|Vtk-m|Vtkm)/\1Viskores/g
s/(\/\/\/?.*)(VTKM|VTK_M)/\1VISKORES/g
EOF

function usage()
{
  echo "Usage: $0 [-f <TARGET_FILE>] [-p <DATABASE>]"
  echo "    -f <TARGET_FILE> Translation unit where to perform the translation"
  echo "    -p <DATABASE>  json file with build rules for the project generated with 'cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON'"
  exit 0
}

function die()
{
  echo "ERROR: $*"
  usage
}


while getopts "f:p:" o
do
  case "${o}" in
    f)
      TARGET_FILE="${OPTARG}"
      ;;

    p)
      DATABASE="${OPTARG}"
      ;;

    *)
      usage
      ;;
  esac
done

test -z "$TARGET_FILE" && die "TARGET_FILE parameter required"
test -z "$DATABASE" && die "DATABASE parameter required"


if transformed_file=$(clang-rename --force --input=<(echo "${CPPRULES}") -p "${DATABASE}" "${TARGET_FILE}")
then
  sed -E -f <(echo "${SEDRULES}") <<<"${transformed_file}"
else
  "${SCRIPT_DIRNAME}/vtkm_to_viskores_file.bash" -f "${TARGET_FILE}"
fi
