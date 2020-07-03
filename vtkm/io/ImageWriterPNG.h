//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageWriterPNG_h
#define vtk_m_io_ImageWriterPNG_h

#include <vtkm/io/ImageWriterBase.h>

namespace vtkm
{
namespace io
{

/// \brief Manages writing images using the PNG format via lodepng
///
/// \c ImageWriterPNG extends vtkm::io::ImageWriterBase and implements writing images in a valid
/// PNG format.  It utilizes lodepng's encode file functions to write
/// PNG images that are automatically compressed to optimal sizes relative to
/// the actual bit complexity of the image.
///
class VTKM_IO_EXPORT ImageWriterPNG : public vtkm::io::ImageWriterBase
{
  using Superclass = vtkm::io::ImageWriterBase;

public:
  using Superclass::Superclass;
  VTKM_CONT ~ImageWriterPNG() noexcept override;
  ImageWriterPNG(const ImageWriterPNG&) = delete;
  ImageWriterPNG& operator=(const ImageWriterPNG&) = delete;

protected:
  VTKM_CONT void Write(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels) override;

  template <typename PixelType>
  VTKM_CONT void WriteToFile(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels);
};
}
} // namespace vtkm::io

#endif //vtk_m_io_ImageWriterPNG_h
