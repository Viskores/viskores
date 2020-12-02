//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_AveragePointNeighborhood_h
#define vtk_m_worklet_AveragePointNeighborhood_h

#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/worklet/WorkletPointNeighborhood.h>

namespace vtkm
{
namespace worklet
{

class AveragePointNeighborhood : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInNeighborhood inputField,
                                FieldOut outputField);
  using ExecutionSignature = _3(_2, Boundary);
  using InputDomain = _1;

  AveragePointNeighborhood(vtkm::IdComponent radius)
  {
    VTKM_ASSERT(radius > 0);
    this->BoundaryRadius = radius;
  }

  template <typename InputFieldPortalType>
  VTKM_EXEC typename InputFieldPortalType::ValueType operator()(
    const vtkm::exec::FieldNeighborhood<InputFieldPortalType>& inputField,
    const vtkm::exec::BoundaryState& boundary) const
  {
    using T = typename InputFieldPortalType::ValueType;

    auto minIndices = boundary.MinNeighborIndices(this->BoundaryRadius);
    auto maxIndices = boundary.MaxNeighborIndices(this->BoundaryRadius);

    T sum(0);
    vtkm::IdComponent size = 0;
    for (vtkm::IdComponent i = minIndices[0]; i <= maxIndices[0]; i++)
    {
      for (vtkm::IdComponent j = minIndices[1]; j <= maxIndices[1]; j++)
      {
        for (vtkm::IdComponent k = minIndices[2]; k <= maxIndices[2]; k++)
        {
          sum = sum + inputField.Get(i, j, k);
          size++;
        }
      }
    }
    return static_cast<T>(sum / size);
  }

private:
  vtkm::IdComponent BoundaryRadius;
};

} // vtkm::worklet
} // vtkm

#endif // vtk_m_worklet_AveragePointNeighborhood_h
