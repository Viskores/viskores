//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_PointAverage_h
#define vtk_m_worklet_PointAverage_h

#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace worklet
{

//simple functor that returns the average point value of a given
//cell based field.
class PointAverage : public vtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInCell inCellField,
                                FieldOutPoint outPointField);
  using ExecutionSignature = void(CellCount, _2, _3);
  using InputDomain = _1;

  template <typename CellValueVecType, typename OutType>
  VTKM_EXEC void operator()(const vtkm::IdComponent& numCells,
                            const CellValueVecType& cellValues,
                            OutType& average) const
  {
    using CellValueType = typename CellValueVecType::ComponentType;

    VTKM_ASSERT(vtkm::VecTraits<CellValueType>::GetNumberOfComponents(cellValues[0]) ==
                vtkm::VecTraits<OutType>::GetNumberOfComponents(average));

    average = cellValues[0];
    for (vtkm::IdComponent cellIndex = 1; cellIndex < numCells; ++cellIndex)
    {
      average += cellValues[cellIndex];
    }

    using VTraits = vtkm::VecTraits<OutType>;
    using OutComponentType = typename vtkm::VecTraits<OutType>::ComponentType;
    const vtkm::IdComponent numComponents = VTraits::GetNumberOfComponents(average);
    for (vtkm::IdComponent compIndex = 0; compIndex < numComponents; ++compIndex)
    {
      VTraits::SetComponent(
        average,
        compIndex,
        static_cast<OutComponentType>(VTraits::GetComponent(average, compIndex) / numCells));
    }
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_PointAverage_h
