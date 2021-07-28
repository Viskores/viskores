//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_PointLocator_h
#define vtk_m_exec_PointLocator_h

#include <vtkm/VirtualObjectBase.h>

#ifdef VTKM_NO_DEPRECATED_VIRTUAL
#error "PointLocator with virtual methods is removed. Do not include PointLocator.h"
#endif

VTKM_DEPRECATED_SUPPRESS_BEGIN

namespace vtkm
{
namespace exec
{

class VTKM_DEPRECATED(1.6, "PointLocator with virtual methods no longer supported.")
  VTKM_ALWAYS_EXPORT PointLocator : public vtkm::VirtualObjectBase
{
  VTKM_DEPRECATED_SUPPRESS_BEGIN
public:
  VTKM_EXEC_CONT virtual ~PointLocator() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC
  virtual void FindNearestNeighbor(const vtkm::Vec3f& queryPoint,
                                   vtkm::Id& pointId,
                                   vtkm::FloatDefault& distanceSquared) const = 0;
  VTKM_DEPRECATED_SUPPRESS_END
};

} // vtkm::exec
} // vtkm

VTKM_DEPRECATED_SUPPRESS_END

#endif // vtk_m_exec_PointLocator_h
