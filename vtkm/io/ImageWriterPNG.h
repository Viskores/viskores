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

/// @brief Writes images using the PNG format.
///
/// `ImageWriterPNG` is constructed with the name of the file to write. The data
/// is written to the file by calling the `WriteDataSet` method.
///
/// When writing files, `ImageReaderPNG` automatically compresses data to optimal
/// sizes relative to the actual bit complexity of the provided image.
///
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
