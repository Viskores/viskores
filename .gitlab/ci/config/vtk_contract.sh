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

# Fetch the latest VTK release tag
readonly vtk_version="v9.4.0"
readonly tarball="$vtk_version.tar.gz"
readonly url="https://github.com/Kitware/VTK/archive/refs/tags/$tarball"
readonly source_dir="$HOME/vtk-source"
readonly build_dir="$HOME/vtk-build"
readonly install_dir="$HOME/vtk-install"

cd "$HOME"

# Download VTK source
curl --insecure -OL "$url"
tar xf "$tarball"
mv VTK-* "$source_dir"

# Create build directory
mkdir -p "$build_dir"

# Configure VTK with minimal build options
cmake -GNinja \
  -S "$source_dir" \
  -B "$build_dir" \
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
  -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters:STRING=YES \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$install_dir"

# Build VTK
cmake --build "$build_dir" --parallel

# Install VTK (optional, but good for verification)
cmake --install "$build_dir"

echo "VTK contract test build completed successfully"
