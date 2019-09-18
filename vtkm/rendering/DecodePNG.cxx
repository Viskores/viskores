//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/DecodePNG.h>

#include <vtkm/cont/Logging.h>
#include <vtkm/internal/Configure.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_DISK
#include <vtkm/thirdparty/lodepng/vtkmlodepng/lodepng.cpp>
VTKM_THIRDPARTY_PRE_INCLUDE

namespace vtkm
{
namespace rendering
{

vtkm::UInt32 DecodePNG(std::vector<unsigned char>& out_image,
                       unsigned long& image_width,
                       unsigned long& image_height,
                       const unsigned char* in_png,
                       std::size_t in_size)
{
  using namespace vtkm::png;
  constexpr std::size_t bitdepth = 8;
  vtkm::UInt32 iw = 0;
  vtkm::UInt32 ih = 0;

  auto retcode = lodepng::decode(out_image, iw, ih, in_png, in_size, LCT_RGBA, bitdepth);
  image_width = iw;
  image_height = ih;
  return retcode;
}
}
} // namespace vtkm::rendering
