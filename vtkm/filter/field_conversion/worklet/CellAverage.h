//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_CellAverage_h
#define vtk_m_worklet_CellAverage_h

#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace worklet
{

//simple functor that returns the average point value as a cell field
class CellAverage : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldInPoint inPoints, FieldOutCell outCells);
  using ExecutionSignature = void(PointCount, _2, _3);
  using InputDomain = _1;

  template <typename PointValueVecType, typename OutType>
  VTKM_EXEC void operator()(const vtkm::IdComponent& numPoints,
                            const PointValueVecType& pointValues,
                            OutType& average) const
  {
    using PointValueType = typename PointValueVecType::ComponentType;

    VTKM_ASSERT(vtkm::VecTraits<PointValueType>::GetNumberOfComponents(pointValues[0]) ==
                vtkm::VecTraits<OutType>::GetNumberOfComponents(average));

    average = pointValues[0];
    for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; ++pointIndex)
    {
      average += pointValues[pointIndex];
    }

    using VTraits = vtkm::VecTraits<OutType>;
    using OutComponentType = typename VTraits::ComponentType;
    const vtkm::IdComponent numComponents = VTraits::GetNumberOfComponents(average);
    for (vtkm::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      VTraits::SetComponent(
        average,
        cIndex,
        static_cast<OutComponentType>(VTraits::GetComponent(average, cIndex) / numPoints));
    }
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_CellAverage_h
