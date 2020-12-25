//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageReaderHDF5_IM_h
#define vtk_m_io_ImageReaderHDF5_IM_h

#include <vtkm/io/ImageReaderBase.h>

#include <hdf5.h>
#include <hdf5_hl.h>

namespace vtkm
{
namespace io
{
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
#endif // vvtk_m_io_ImageReaderHDF5_IM_h
