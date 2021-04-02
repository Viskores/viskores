//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageReaderPNG_h
#define vtk_m_io_ImageReaderPNG_h

#include <vtkm/io/ImageReaderBase.h>

namespace vtkm
{
namespace io
{

/// \brief Manages reading images using the PNG format via lodepng
///
/// `ImageReaderPNG` extends `ImageReaderBase` and implements reading images in a valid
/// PNG format.  It utilizes lodepng's decode file functions to read
/// PNG images that are automatically compressed to optimal sizes relative to
/// the actual bit complexity of the image.
///
/// `ImageReaderPNG` will automatically upsample/downsample read image data
/// to a 16 bit RGB no matter how the image is compressed. It is up to the user to
/// decide the pixel format for input PNGs
class VTKM_IO_EXPORT ImageReaderPNG : public ImageReaderBase
{
  using Superclass = ImageReaderBase;

public:
  using Superclass::Superclass;
  VTKM_CONT ~ImageReaderPNG() noexcept override;
  ImageReaderPNG(const ImageReaderPNG&) = delete;
  ImageReaderPNG& operator=(const ImageReaderPNG&) = delete;

protected:
  VTKM_CONT void Read() override;
};
}
} // namespace vtkm::io

#endif //vtk_m_io_ImageReaderPNG_h
