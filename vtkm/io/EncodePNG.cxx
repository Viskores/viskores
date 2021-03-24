//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/io/EncodePNG.h>
#include <vtkm/io/FileUtils.h>

#include <vtkm/cont/Logging.h>
#include <vtkm/internal/Configure.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/lodepng/vtkmlodepng/lodepng.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace io
{

vtkm::UInt32 EncodePNG(std::vector<unsigned char> const& image,
                       unsigned long width,
                       unsigned long height,
                       std::vector<unsigned char>& output_png)
{
  // The default is 8 bit RGBA; does anyone care to have more options?
  // We can certainly add them in a backwards-compatible way if need be.
  vtkm::UInt32 error = vtkm::png::lodepng::encode(
    output_png, image, static_cast<unsigned int>(width), static_cast<unsigned int>(height));
  if (error)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "LodePNG Encoder error number " << error << ": " << png::lodepng_error_text(error));
  }
  return error;
}


vtkm::UInt32 SavePNG(std::string const& filename,
                     std::vector<unsigned char> const& image,
                     unsigned long width,
                     unsigned long height)
{
  if (!vtkm::io::EndsWith(filename, ".png"))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Error,
               "File " << filename << " does not end with .png; this is required.");
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
} // namespace vtkm::io
