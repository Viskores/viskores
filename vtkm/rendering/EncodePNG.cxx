//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <algorithm> // for std::equal
#include <vtkm/rendering/EncodePNG.h>

#include <vtkm/cont/Logging.h>
#include <vtkm/internal/Configure.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/lodepng/vtkmlodepng/lodepng.cpp>
VTKM_THIRDPARTY_PRE_INCLUDE

namespace vtkm
{
namespace rendering
{

vtkm::UInt32 EncodePNG(std::vector<unsigned char> const& image,
                       unsigned long width,
                       unsigned long height,
                       std::vector<unsigned char>& output_png)
{
  // The default is 8 bit RGBA; does anyone care to have more options?
  vtkm::UInt32 error = vtkm::png::lodepng::encode(output_png, image, width, height);
  if (error)
  {
    // TODO: Use logging framework instead:
    std::cerr << "PNG Encoder error number " << error << ": " << png::lodepng_error_text(error)
              << "\n";
  }
  return error;
}


vtkm::UInt32 SavePNG(std::string const& filename,
                     std::vector<unsigned char> const& image,
                     unsigned long width,
                     unsigned long height)
{
  auto ends_with = [](std::string const& value, std::string const& ending) {
    if (ending.size() > value.size())
    {
      return false;
    }
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
  };

  if (!ends_with(filename, ".png"))
  {
    std::cerr << "PNG filename must end with .png\n";
  }

  std::vector<unsigned char> output_png;
  vtkm::UInt32 error = EncodePNG(image, width, height, output_png);
  if (!error)
  {
    vtkm::png::lodepng::save_file(output_png, filename);
  }
  return error;
}
}
} // namespace vtkm::rendering
