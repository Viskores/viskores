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

namespace vtkm
{
namespace exec
{

class VTKM_ALWAYS_EXPORT PointLocator : public vtkm::VirtualObjectBase
{
public:
  VTKM_EXEC_CONT virtual ~PointLocator() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC virtual void FindNearestNeighbor(const vtkm::Vec3f& queryPoint,
                                             vtkm::Id& pointId,
                                             vtkm::FloatDefault& distanceSquared) const = 0;
};
}
}
#endif // vtk_m_exec_PointLocator_h
