#!/bin/bash

##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

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

set -xe

# Build Viskores first with VTK types
readonly viskores_source_dir="$CI_PROJECT_DIR"
readonly viskores_build_dir="$HOME/viskores-build"
readonly viskores_install_dir="$HOME/viskores-install"

# Configure Viskores with VTK types
cmake -GNinja \
  -S "$viskores_source_dir" \
  -B "$viskores_build_dir" \
  -DBUILD_SHARED_LIBS=ON \
  -DViskores_USE_DEFAULT_TYPES_FOR_VTK=ON \
  -DViskores_ENABLE_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$viskores_install_dir"

# Build and install Viskores
cmake --build "$viskores_build_dir" --parallel
cmake --install "$viskores_build_dir"

# Now build VTK with the Viskores installation
readonly vtk_version="v9.5.2"
readonly tarball="$vtk_version.tar.gz"
readonly url="https://github.com/Kitware/VTK/archive/refs/tags/$tarball"
readonly vtk_source_dir="$HOME/vtk-source"
readonly vtk_build_dir="$HOME/vtk-build"
readonly vtk_install_dir="$HOME/vtk-install"

cd "$HOME"

# Download VTK source
curl --insecure -OL "$url"
tar xf "$tarball"
mv VTK-* "$vtk_source_dir"

# Configure VTK with minimal build options and Viskores
cmake -GNinja \
  -S "$vtk_source_dir" \
  -B "$vtk_build_dir" \
  -DBUILD_SHARED_LIBS=ON \
  -DVTK_BUILD_TESTING=OFF \
  -DVTK_BUILD_EXAMPLES=OFF \
  -DVTK_ENABLE_WRAPPING=OFF \
  -DVTK_ENABLE_REMOTE_MODULES=OFF \
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
  -DCMAKE_INSTALL_PREFIX="$vtk_install_dir"

# Build VTK
cmake --build "$vtk_build_dir" --parallel

# Install VTK (optional, but good for verification)
cmake --install "$vtk_build_dir"

echo "VTK contract test build completed successfully"
