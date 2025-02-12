#!/bin/bash

set -x
SCRIPT_DIRNAME=$(dirname "$0")

function usage()
{
  echo "Usage: $0 -i <INPUT_DIR> -o <OUTPUT_DIR> -p <DATABASE>"
  echo "    -i <INPUT_DIR> Directory containing vtk-m source code"
  echo "    -o <OUTPUT_DIR> Directory where the converted code will be generated"
  echo "    -p <DATABASE> JSON file with build rules for the project generated with 'cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON'"
  echo "    -j <JOBS> number of simultaneous processes to use for clang-rename"
  exit 0
}

function die()
{
  echo "ERROR: $*"
  usage
}

command -v clang-rename &> /dev/null || die "clang-rename needed"
command -v parallel &> /dev/null || die "GNU parallel needed"
command -v rsync &> /dev/null || die "rsync needed"

NJOBS=16
while getopts "i:o:p:j:" o
do
  case "${o}" in
    i)
      INPUT_DIR="${OPTARG}"
      ;;
    o)
      OUTPUT_DIR="${OPTARG}"
      ;;
    p)
      DATABASE="${OPTARG}"
      ;;
    j)
      NJOBS="${OPTARG}"
      ;;
    *)
      usage
      ;;
  esac
done

test -z "${INPUT_DIR}" && die "INPUT_DIR parameter required"
test -z "${OUTPUT_DIR}" && die "OUTPUT_DIR parameter required"

# Copy source code
#rsync -av --exclude=.git --ignore-existing "${INPUT_DIR}/" "${OUTPUT_DIR}"

cpp_sources=$(find "${INPUT_DIR}" \
  \( \
  -iname '*.cc' \
  -o -iname '*.cpp' \
  -o -iname '*.cu' \
  -o -iname '*.cxx' \
  \) \
  -exec realpath --relative-to="${INPUT_DIR}" '{}' \;)

parallel --progress -j"${NJOBS}" "${SCRIPT_DIRNAME}/vtkm_to_viskores_cpp_src.bash" -p "${DATABASE}" -f "${INPUT_DIR}/{}" ">" "${OUTPUT_DIR}/{}" ::: "${cpp_sources[@]}"

other_sources=$(find "${INPUT_DIR}" \
  -iname '.bash' \
  -o -iname '*.cc.in' \
  -o -iname '*.cpp.in' \
  -o -iname '*.cu.in' \
  -o -iname '*.cxx.in' \
  -o -iname '*.h.in' \
  -o -iname '*.hpp.in' \
  -o -iname '*.hxx.in' \
  -o -iname '*.h' \
  -o -iname '*.hpp' \
  -o -iname '*.hxx' \
  -o -iname '*.cmake' \
  -o -iname '*.in' \
  -o -iname '*.json' \
  -o -iname '*.md' \
  -o -iname '*.module' \
  -o -iname '*.py' \
  -o -iname '*.rst' \
  -o -iname '*.sh' \
  -o -iname '*.supp' \
  -o -iname '*.tmpl' \
  -o -iname '*.txt')

for src in ${other_sources[@]}
do
  relative_path="$(realpath --relative-to="${INPUT_DIR}" "${src}")"
  "${SCRIPT_DIRNAME}/vtkm_to_viskores_file.bash" -f "${src}" > "${OUTPUT_DIR}/${relative_path}"
done
