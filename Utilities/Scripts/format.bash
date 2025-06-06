#!/usr/bin/env bash

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

# Check C/CUDA/C++ code with clang-format
# shellcheck disable=SC2016
find \
  benchmarking \
  docs/users-guide/examples \
  examples \
  viskores \
  viskoresstd \
  \( \
  -iname '*.cc' \
  -o -iname '*.cpp' \
  -o -iname '*.cu' \
  -o -iname '*.cxx' \
  -o -iname '*.h' \
  -o -iname '*.hpp' \
  -o -iname '*.hxx' \
  \) \
  -not -path '*thirdparty*' \
  -print0 \
  | xargs -n1 -0 bash -c 'test -f $0.in || echo $0' | xargs clang-format-16 -i

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
