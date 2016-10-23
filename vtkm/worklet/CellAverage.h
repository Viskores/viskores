//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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

namespace vtkm {
namespace worklet {

//simple functor that returns the average point value.
class CellAverage :
        public vtkm::worklet::WorkletMapPointToCell
{
public:
  typedef void ControlSignature(FieldInPoint<> inPoints,
                                CellSetIn cellset,
                                FieldOutCell<> outCells);
  typedef void ExecutionSignature(_1, PointCount, _3);
  typedef _2 InputDomain;

  template<typename PointValueVecType, typename OutType>
  VTKM_EXEC_EXPORT
  void operator()(const PointValueVecType &pointValues,
                  const vtkm::IdComponent &numPoints,
                  OutType &average) const
  {
    using PointValueType = typename PointValueVecType::ComponentType;
    using PointVecSize =
        std::integral_constant<
          vtkm::IdComponent,
          vtkm::VecTraits<PointValueType>::NUM_COMPONENTS>;
    using OutVecSize =
        std::integral_constant<
          vtkm::IdComponent,
          vtkm::VecTraits<OutType>::NUM_COMPONENTS>;

    this->DoAverage(pointValues,
                    numPoints,
                    average,
                    PointVecSize(),
                    OutVecSize());
  }

private:
  template<typename PointValueVecType, typename OutType>
  VTKM_EXEC_EXPORT
  void DoAverage(const PointValueVecType &pointValues,
                 const vtkm::IdComponent &numPoints,
                 OutType &average,
                 std::integral_constant<vtkm::IdComponent,1>,
                 std::integral_constant<vtkm::IdComponent,1>) const
  {
    OutType sum = static_cast<OutType>(pointValues[0]);
    for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; ++pointIndex)
      {
      sum = sum + static_cast<OutType>(pointValues[pointIndex]);
      }

    average = sum / static_cast<OutType>(numPoints);
  }

  template<typename PointValueVecType,
           typename OutType,
           vtkm::IdComponent VecSize>
  VTKM_EXEC_EXPORT
  void DoAverage(const PointValueVecType &pointValues,
                 const vtkm::IdComponent &numPoints,
                 OutType &average,
                 std::integral_constant<vtkm::IdComponent,VecSize>,
                 std::integral_constant<vtkm::IdComponent,VecSize>) const
  {
    OutType sum = OutType(pointValues[0]);
    for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; ++pointIndex)
      {
      sum = sum + OutType(pointValues[pointIndex]);
      }

    average = sum / OutType(numPoints);
  }

  template<typename PointValueVecType,
           typename OutType,
           vtkm::IdComponent InVecSize,
           vtkm::IdComponent OutVecSize>
  VTKM_EXEC_EXPORT
  void DoAverage(const PointValueVecType &vtkmNotUsed(pointValues),
                 const vtkm::IdComponent &vtkmNotUsed(numPoints),
                 OutType &vtkmNotUsed(average),
                 std::integral_constant<vtkm::IdComponent,InVecSize>,
                 std::integral_constant<vtkm::IdComponent,OutVecSize>) const
  {
    this->RaiseError(
          "CellAverage called with mismatched Vec sizes for CellAverage.");
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_CellAverage_h
