//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageWriterPNM_h
#define vtk_m_io_ImageWriterPNM_h

#include <vtkm/io/ImageWriterBase.h>

namespace vtkm
{
namespace io
{

/// \brief Writes images using the PNM format.
///
/// `ImageWriterPNM` is constructed with the name of the file to write. The data
/// is written to the file by calling the `WriteDataSet` method.
///
/// `ImageWriterPNM` writes images in PNM format (for magic number P6).
/// These files are most commonly stored with a `.ppm` extension although the
/// `.pnm` extension is also valid. More details on the PNM format can be found at
/// http://netpbm.sourceforge.net/doc/ppm.html
///
class VTKM_IO_EXPORT ImageWriterPNM : public vtkm::io::ImageWriterBase
{
  using Superclass = vtkm::io::ImageWriterBase;

public:
  using Superclass::Superclass;
  VTKM_CONT ~ImageWriterPNM() noexcept override;
  ImageWriterPNM(const ImageWriterPNM&) = delete;
  ImageWriterPNM& operator=(const ImageWriterPNM&) = delete;

  /// Attempts to write the ImageDataSet to a PNM file. The MaxColorValue
  /// set in the file with either be selected from the stored MaxColorValue
  /// member variable, or from the templated type if MaxColorValue hasn't been
  /// set from a read file.
  ///
  VTKM_CONT void Write(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels) override;

protected:
  template <typename PixelType>
  VTKM_CONT void WriteToFile(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels);
};
}
} // namespace vtkm::io

#endif //vtk_m_io_ImageWriterPNM_h
