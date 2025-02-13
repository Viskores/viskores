#!/usr/bin/env bash

# Check C/CUDA/C++ code with clang-format
find \
  benchmarking \
  docs/users-guide/examples \
  examples \
  viskores \
  viskoresstd \
  -iname '*.cc' \
  -o -iname '*.cpp' \
  -o -iname '*.cu' \
  -o -iname '*.cxx' \
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
  | xargs clang-format-11.0.1 -i

DIFF="$(git diff)"
if [ -n "${DIFF}" ]
then
  echo "clang-format:"
  echo "  Code format checks failed."
  echo "  Please run clang-format v16 your changes before committing:"
  echo "  You can use our CI image for this with: Utilities/Script/format.bash"
  echo "  The following changes are suggested:"
  echo "${DIFF}"
  git diff --stat
  exit 1
fi

exit 0
