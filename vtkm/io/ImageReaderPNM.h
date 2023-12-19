//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageReaderPNM_h
#define vtk_m_io_ImageReaderPNM_h

#include <vtkm/io/ImageReaderBase.h>

namespace vtkm
{
namespace io
{

/// \brief Reads images using the PNM format.
///
/// `ImageReaderPNM` is constructed with the name of the file to read. The data
/// from the file is read by calling the `ReadDataSet` method.
///
/// Currently, `ImageReaderPNM` only supports files using the portable pixmap (PPM)
/// format (with magic number ``P6''). These files are most commonly stored with a
/// `.ppm` extension although the `.pnm` extension is also valid.
/// More details on the PNM format can be found here at
/// http://netpbm.sourceforge.net/doc/ppm.html
///
/// By default, the colors are stored in a field named "colors", but the name of the
/// field can optionally be changed using the `SetPointFieldName` method.
///
class VTKM_IO_EXPORT ImageReaderPNM : public ImageReaderBase
{
  using Superclass = ImageReaderBase;

public:
  using Superclass::Superclass;
  VTKM_CONT ~ImageReaderPNM() noexcept override;
  ImageReaderPNM(const ImageReaderPNM&) = delete;
  ImageReaderPNM& operator=(const ImageReaderPNM&) = delete;

protected:
  VTKM_CONT void Read() override;

  /// Reads image data from the provided inStream with the supplied width/height
  /// Stores the data in a vector of PixelType which is converted to an DataSet
  ///
  template <typename PixelType>
  void DecodeFile(std::ifstream& inStream, const vtkm::Id& width, const vtkm::Id& height);
};
}
} // namespace vtkm::io

#endif //vtk_m_io_ImageReaderPNM_h
