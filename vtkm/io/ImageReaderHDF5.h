//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageReaderHDF5_h
#define vtk_m_io_ImageReaderHDF5_h

#include <vtkm/io/ImageReaderBase.h>

namespace vtkm
{
namespace io
{
/// \brief Reading images using HDF5 Image format
///
/// \c ImageReaderHDF5 extends vtkm::io::ImageWriterBase and implements writing image
/// HDF5 file format. It conforms to the HDF5 Image Specification
/// https://portal.hdfgroup.org/display/HDF5/HDF5+Image+and+Palette+Specification%2C+Version+1.2
class VTKM_IO_EXPORT ImageReaderHDF5 : public ImageReaderBase
{
  using Superclass = ImageReaderBase;

public:
  using Superclass::Superclass;
  VTKM_CONT ~ImageReaderHDF5() noexcept override;
  ImageReaderHDF5(const ImageReaderHDF5&) = delete;
  ImageReaderHDF5& operator=(const ImageReaderHDF5&) = delete;

protected:
  VTKM_CONT void Read() override;

private:
};
}
}
#endif // vvtk_m_io_ImageReaderHDF5_h
