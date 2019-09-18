//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_openmp_internal_AtomicInterfaceExecutionOpenMP_h
#define vtk_m_cont_openmp_internal_AtomicInterfaceExecutionOpenMP_h

#include <vtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#include <vtkm/cont/internal/AtomicInterfaceControl.h>
#include <vtkm/cont/internal/AtomicInterfaceExecution.h>

#include <vtkm/Types.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <>
class AtomicInterfaceExecution<DeviceAdapterTagOpenMP> : public AtomicInterfaceControl
{
};
}
}
} // end namespace vtkm::cont::internal

#endif // vtk_m_cont_openmp_internal_AtomicInterfaceExecutionOpenMP_h
