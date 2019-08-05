//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_CellLocator_h
#define vtk_m_exec_CellLocator_h

#include <vtkm/Types.h>
#include <vtkm/VirtualObjectBase.h>
#include <vtkm/exec/FunctorBase.h>

namespace vtkm
{
namespace exec
{

class VTKM_ALWAYS_EXPORT CellLocator : public vtkm::VirtualObjectBase
{
public:
  VTKM_EXEC_CONT virtual ~CellLocator() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC
  virtual void FindCell(const vtkm::Vec3f& point,
                        vtkm::Id& cellId,
                        vtkm::Vec3f& parametric,
                        const vtkm::exec::FunctorBase& worklet) const = 0;
};

} // namespace exec
} // namespace vtkm

#endif // vtk_m_exec_CellLocator_h
