//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/PixelTypes.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/lodepng/vtkmlodepng/lodepng.h>
VTKM_THIRDPARTY_POST_INCLUDE

int vtkm::io::internal::GreyColorType = static_cast<int>(vtkm::png::LodePNGColorType::LCT_GREY);

int vtkm::io::internal::RGBColorType = static_cast<int>(vtkm::png::LodePNGColorType::LCT_RGB);
