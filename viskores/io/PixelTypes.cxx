//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/io/PixelTypes.h>

VISKORES_THIRDPARTY_PRE_INCLUDE
#include <viskores/thirdparty/lodepng/viskoreslodepng/lodepng.h>
VISKORES_THIRDPARTY_POST_INCLUDE

int viskores::io::internal::GreyColorType =
  static_cast<int>(viskores::png::LodePNGColorType::LCT_GREY);

int viskores::io::internal::RGBColorType =
  static_cast<int>(viskores::png::LodePNGColorType::LCT_RGB);
