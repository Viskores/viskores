#!/bin/bash

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

set -xe

# Viskores is pre-built by an upstream job and passed as an artifact tarball.
readonly viskores_install_dir="${CI_PROJECT_DIR}/viskores-install"
tar xf "${CI_PROJECT_DIR}/viskores-install.tar.gz" -C "${CI_PROJECT_DIR}"

readonly vtk_version="v9.6.2"
readonly vtk_fix_commit="d8c01e95d1524fdb0faf49bd451907d8882de098"
readonly vtk_repo="https://gitlab.kitware.com/vtk/vtk.git"
readonly vtk_source_dir="$HOME/vtk-source"
readonly vtk_build_dir="$HOME/vtk-build"
readonly vtk_install_dir="$HOME/vtk-install"

# Clone VTK at the pinned tag then cherry-pick the upstream fix for
# Viskores device source marking (PR #161 compat). Drop the cherry-pick
# once a VTK release containing vtk_add_viskores_device_target_information()
# is used.
git clone --branch "$vtk_version" "$vtk_repo" "$vtk_source_dir"
git -C "$vtk_source_dir" fetch origin "$vtk_fix_commit"
git -C "$vtk_source_dir" config user.email "viskores-ci@viskores.org"
git -C "$vtk_source_dir" config user.name "Viskores CI"
git -C "$vtk_source_dir" cherry-pick -Xtheirs -m1 "$vtk_fix_commit"

# Configure VTK with minimal build options and Viskores
cmake -GNinja \
  -S "$vtk_source_dir" \
  -B "$vtk_build_dir" \
  -DBUILD_SHARED_LIBS=ON \
  -DVTK_BUILD_TESTING=OFF \
  -DVTK_BUILD_EXAMPLES=OFF \
  -DVTK_ENABLE_WRAPPING=OFF \
  -DVTK_ENABLE_REMOTE_MODULES=OFF \
  -DVTK_ENABLE_VISKORES_OVERRIDES=ON \
  -DVTK_GROUP_ENABLE_StandAlone=WANT \
  -DVTK_GROUP_ENABLE_Rendering=DONT_WANT \
  -DVTK_GROUP_ENABLE_Imaging=DONT_WANT \
  -DVTK_GROUP_ENABLE_MPI=NO \
  -DVTK_GROUP_ENABLE_Qt=NO \
  -DVTK_GROUP_ENABLE_Tk=NO \
  -DVTK_GROUP_ENABLE_Web=NO \
  -DVTK_MODULE_USE_EXTERNAL_VTK_vtkviskores=ON \
  -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters:STRING=YES \
  -DViskores_ROOT="$viskores_install_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_INSTALL_PREFIX="$vtk_install_dir" \
  ${CMAKE_ARGS:-}

# Build VTK
cmake --build "$vtk_build_dir" --parallel

# Install VTK (optional, but good for verification)
cmake --install "$vtk_build_dir"

echo "VTK contract test build completed successfully"
