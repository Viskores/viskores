//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageWriterHDF5_H
#define vtk_m_io_ImageWriterHDF5_H

#include <vtkm/io/ImageWriterBase.h>

namespace vtkm
{
namespace io
{

/// \brief Writing images using HDF5 Image format
///
/// \c ImageWriterHDF5 extends vtkm::io::ImageWriterBase and implements writing image
/// HDF5 file format. It conforms to the HDF5 Image Specification
/// https://portal.hdfgroup.org/display/HDF5/HDF5+Image+and+Palette+Specification%2C+Version+1.2
class VTKM_IO_EXPORT ImageWriterHDF5 : public vtkm::io::ImageWriterBase
{
  using Superclass = vtkm::io::ImageWriterBase;

public:
  using Superclass::Superclass;
  VTKM_CONT ~ImageWriterHDF5() noexcept override;
  ImageWriterHDF5(const ImageWriterHDF5&) = delete;
  ImageWriterHDF5& operator=(const ImageWriterHDF5&) = delete;

  VTKM_CONT void WriteDataSet(const vtkm::cont::DataSet& dataSet,
                              const std::string& colorField = {});

protected:
  VTKM_CONT void Write(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels) override;

private:
  template <typename PixelType>
  VTKM_CONT int WriteToFile(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels);

  std::int64_t fileid = 0;
  // FIXME: a hack for the moment, design a better API.
  std::string fieldName;

  static constexpr auto IMAGE_CLASS = "IMAGE";
  static constexpr auto IMAGE_VERSION = "1.2";
};
}
}
#endif //vtk_m_io_ImageWriterHDF5_H
