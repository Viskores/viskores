//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define vtk_m_cont_tbb_internal_ArrayManagerExecutionTBB_cxx

#include <vtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>

namespace vtkm
{
namespace cont
{

VTKM_INSTANTIATE_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagTBB)
}
} // end vtkm::cont
