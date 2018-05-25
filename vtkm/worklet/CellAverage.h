//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
class CellAverage : public vtkm::worklet::WorkletMapPointToCell
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint<> inPoints,
                                FieldOutCell<> outCells);
  using ExecutionSignature = void(PointCount, _2, _3);
  using InputDomain = _1;

  template <typename PointValueVecType, typename OutType>
  VTKM_EXEC void operator()(const vtkm::IdComponent& numPoints,
                            const PointValueVecType& pointValues,
                            OutType& average) const
  {
    using PointValueType = typename PointValueVecType::ComponentType;

    using InVecSize =
      std::integral_constant<vtkm::IdComponent, vtkm::VecTraits<PointValueType>::NUM_COMPONENTS>;
    using OutVecSize =
      std::integral_constant<vtkm::IdComponent, vtkm::VecTraits<OutType>::NUM_COMPONENTS>;
    using SameLengthVectors = typename std::is_same<InVecSize, OutVecSize>::type;

    this->DoAverage(numPoints, pointValues, average, SameLengthVectors());
  }

private:
  template <typename PointValueVecType, typename OutType>
  VTKM_EXEC void DoAverage(const vtkm::IdComponent& numPoints,
                           const PointValueVecType& pointValues,
                           OutType& average,
                           std::true_type) const
  {
    using OutComponentType = typename vtkm::VecTraits<OutType>::ComponentType;
    OutType sum = OutType(pointValues[0]);
    for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; ++pointIndex)
    {
      sum = sum + OutType(pointValues[pointIndex]);
    }

    average = sum / OutType(static_cast<OutComponentType>(numPoints));
  }

  template <typename PointValueVecType, typename OutType>
  VTKM_EXEC void DoAverage(const vtkm::IdComponent& vtkmNotUsed(numPoints),
                           const PointValueVecType& vtkmNotUsed(pointValues),
                           OutType& vtkmNotUsed(average),
                           std::false_type) const
  {
    this->RaiseError("CellAverage called with mismatched Vec sizes for CellAverage.");
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_CellAverage_h
