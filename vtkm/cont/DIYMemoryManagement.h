//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_DIYMemoryManagement_h
#define vtk_m_cont_internal_DIYMemoryManagement_h

#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/vtkm_cont_export.h>
#include <vtkm/thirdparty/diy/diy.h>

namespace vtkm
{
namespace cont
{

VTKM_CONT_EXPORT vtkm::cont::DeviceAdapterId GetDIYDeviceAdapter();

/// \brief Wraps vtkmdiy::Master::exchange by setting its appropiate vtkmdiy::MemoryManagement.
VTKM_CONT_EXPORT void DIYMasterExchange(vtkmdiy::Master& master, bool remote = false);

}
}

#endif
