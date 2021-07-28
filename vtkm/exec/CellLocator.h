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

#include <vtkm/Deprecated.h>
#include <vtkm/ErrorCode.h>
#include <vtkm/Types.h>
#include <vtkm/VirtualObjectBase.h>
#include <vtkm/exec/FunctorBase.h>

#ifdef VTKM_NO_DEPRECATED_VIRTUAL
#error "CellLocator with virtual methods is removed. Do not include CellLocator.h"
#endif

VTKM_DEPRECATED_SUPPRESS_BEGIN

namespace vtkm
{
namespace exec
{

class VTKM_DEPRECATED(
  1.6,
  "CellLocator with virtual methods no longer supported. Use CellLocatorGeneral.")
  VTKM_ALWAYS_EXPORT CellLocator : public vtkm::VirtualObjectBase
{
  VTKM_DEPRECATED_SUPPRESS_BEGIN
public:
  VTKM_EXEC_CONT virtual ~CellLocator() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC
  virtual vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                                   vtkm::Id& cellId,
                                   vtkm::Vec3f& parametric) const = 0;

  VTKM_DEPRECATED(1.6, "FindCell no longer takes worklet argument.")
  VTKM_EXEC
  void FindCell(const vtkm::Vec3f& point,
                vtkm::Id& cellId,
                vtkm::Vec3f& parametric,
                const vtkm::exec::FunctorBase& worklet) const
  {
    vtkm::ErrorCode status = this->FindCell(point, cellId, parametric);
    if (status != vtkm::ErrorCode::Success)
    {
      worklet.RaiseError(vtkm::ErrorString(status));
    }
  }
  VTKM_DEPRECATED_SUPPRESS_END
};

} // namespace exec
} // namespace vtkm

VTKM_DEPRECATED_SUPPRESS_END

#endif // vtk_m_exec_CellLocator_h
