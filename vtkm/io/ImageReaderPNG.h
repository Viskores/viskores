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

/// @brief Reads images using the PNG format.
///
/// `ImageReaderPNG` is constructed with the name of the file to read. The data
/// from the file is read by calling the `ReadDataSet` method.
///
/// `ImageReaderPNG` will automatically upsample/downsample read image data
/// to a 16 bit RGB no matter how the image is compressed. It is up to the user to
/// decide the pixel format for input PNGs
///
/// By default, the colors are stored in a field named "colors", but the name of the
/// field can optionally be changed using the `SetPointFieldName` method.
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
