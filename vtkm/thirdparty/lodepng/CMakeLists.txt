##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

add_library(vtkm_lodepng OBJECT vtkmlodepng/lodepng.cpp)

# To be consumed by other potentially PIC libraries
set_target_properties(vtkm_lodepng PROPERTIES POSITION_INDEPENDENT_CODE ON)

# This will not install anything but it is needed for CMake <= 3.21 since it
# does not fully support $<TARGET_OBJECTS> in target_link_library.
vtkm_install_targets(TARGETS vtkm_lodepng)
