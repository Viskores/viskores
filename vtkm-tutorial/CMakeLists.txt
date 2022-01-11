##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#add the directory that contains the VTK-m config file to the cmake
#path so that our examples can find VTK-m

set(CMAKE_PREFIX_PATH ${VTKm_BINARY_DIR}/${VTKm_INSTALL_CONFIG_DIR})
add_subdirectory(tutorialExamples)
